import ctypes
import os
import sys
import tkinter as tk
from tkinter import messagebox, ttk
import winreg


#REGISTRY_PATH = r"SOFTWARE\Virtual Desktop, Inc.\OpenXR"
REGISTRY_PATH = r"SOFTWARE\VirtualDesktop-OpenXR"
ENABLE_VALUE_NAME = "DLSSNR_Enabled"
VALUE_NAMES = (
    "DLSSNR_Style",
    "DLSSNR_Intensity",
    "DLSSNR_LocalToneStrength",
    "DLSSNR_LocalStructureStrength",
    "DLSSNR_SkinStructureStrength",
    "FoveationSize",
)
VALUE_MAXIMUMS = (2, 100, 100, 100, 100, 100)
VALUE_DEFAULTS = (0, 100, 20, 70, 50, 66)


def is_elevated():
    try:
        return bool(ctypes.windll.shell32.IsUserAnAdmin())
    except AttributeError:
        return False


def relaunch_as_admin():
    executable = sys.executable
    script = os.path.abspath(sys.argv[0])
    parameters = " ".join(f'"{argument}"' for argument in [script, *sys.argv[1:]])
    result = ctypes.windll.shell32.ShellExecuteW(
        None, "runas", executable, parameters, None, 1
    )
    if result <= 32:
        raise OSError(f"Could not request administrator access (error {result}).")


def read_values():
    values = list(VALUE_DEFAULTS)
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, REGISTRY_PATH) as key:
            for index, name in enumerate(VALUE_NAMES):
                try:
                    value, _ = winreg.QueryValueEx(key, name)
                    values[index] = max(0, min(VALUE_MAXIMUMS[index], int(value)))
                except FileNotFoundError:
                    pass
    except FileNotFoundError:
        pass
    return values


def read_enabled():
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, REGISTRY_PATH) as key:
            value, _ = winreg.QueryValueEx(key, ENABLE_VALUE_NAME)
            return bool(int(value))
    except FileNotFoundError:
        return False


def write_value(name, value):
    maximum = 2 if name == "DLSSNR_Style" else 100
    value = max(0, min(maximum, int(value)))
    with winreg.CreateKeyEx(
        winreg.HKEY_LOCAL_MACHINE, REGISTRY_PATH, 0, winreg.KEY_SET_VALUE
    ) as key:
        winreg.SetValueEx(key, name, 0, winreg.REG_DWORD, int(value))


class VDXRRegistryApp:
    def __init__(self, root):
        self.root = root
        self.root.title("DLSS-NR Settings")
        self.root.resizable(False, False)

        frame = ttk.Frame(root, padding=16)
        frame.grid()

        ttk.Label(frame, text="VDXR registry values", font=("Segoe UI", 12, "bold")).grid(
            row=0, column=0, columnspan=3, sticky="w", pady=(0, 12)
        )

        self.enabled = tk.BooleanVar(value=read_enabled())
        ttk.Checkbutton(
            frame,
            text="Enable DLSS-NR",
            variable=self.enabled,
            command=self.on_enabled_changed,
        ).grid(row=1, column=0, columnspan=3, sticky="w", pady=(0, 8))
        ttk.Button(
            frame,
            text="Reset to defaults",
            command=self.reset_defaults,
        ).grid(row=2, column=0, columnspan=3, sticky="w", pady=(0, 8))

        self.variables = []
        self.slider_widgets = []
        self.foveation_save_id = None
        for index, value in enumerate(read_values(), start=0):
            variable = tk.IntVar(value=value)
            variable.trace_add("write", lambda *_args, i=index, v=variable: self.save(i, v))
            self.variables.append(variable)

            ttk.Label(frame, text=VALUE_NAMES[index], width=31).grid(
                row=index + 3, column=0, sticky="w", padx=(0, 8), pady=4
            )
            tk.Scale(
                frame,
                from_=0,
                to=VALUE_MAXIMUMS[index],
                resolution=1,
                variable=variable,
                orient="horizontal",
                length=260,
                showvalue=False,
                highlightthickness=0,
            ).grid(row=index + 3, column=1, sticky="ew", pady=4)
            self.slider_widgets.append(frame.grid_slaves(row=index + 3, column=1)[0])
            ttk.Label(frame, textvariable=variable, width=4, anchor="e").grid(
                row=index + 3, column=2, sticky="e", padx=(8, 0), pady=4
            )

        self.status = tk.StringVar(value=REGISTRY_PATH)
        ttk.Label(frame, textvariable=self.status).grid(
            row=9, column=0, columnspan=3, sticky="w", pady=(12, 0)
        )
        self.set_slider_state()

    def set_slider_state(self):
        state = tk.NORMAL if self.enabled.get() else tk.DISABLED
        for slider in self.slider_widgets:
            slider.configure(state=state)

    def on_enabled_changed(self):
        try:
            write_value(ENABLE_VALUE_NAME, int(self.enabled.get()))
            self.set_slider_state()
            state = "enabled" if self.enabled.get() else "disabled"
            self.status.set(f"DLSS-NR {state}")
        except PermissionError:
            self.status.set("Administrator access is required to write HKLM.")
        except OSError as error:
            self.status.set(f"Registry error: {error}")

    def reset_defaults(self):
        try:
            self.enabled.set(False)
            write_value(ENABLE_VALUE_NAME, 0)
            for variable, default in zip(self.variables, VALUE_DEFAULTS):
                variable.set(default)
            self.set_slider_state()
            self.status.set("Reset to defaults")
        except PermissionError:
            self.status.set("Administrator access is required to write HKLM.")
        except OSError as error:
            self.status.set(f"Registry error: {error}")

    def save(self, index, variable):
        if VALUE_NAMES[index] == "FoveationSize":
            if self.foveation_save_id is not None:
                self.root.after_cancel(self.foveation_save_id)
            self.foveation_save_id = self.root.after(
                400, self.save_foveation_size, variable
            )
            return

        try:
            write_value(VALUE_NAMES[index], variable.get())
            self.status.set(f"Saved {VALUE_NAMES[index]} = {variable.get()}")
        except PermissionError:
            self.status.set("Administrator access is required to write HKLM.")
        except OSError as error:
            self.status.set(f"Registry error: {error}")

    def save_foveation_size(self, variable):
        self.foveation_save_id = None
        try:
            write_value("FoveationSize", variable.get())
            self.status.set(f"Saved FoveationSize = {variable.get()}")
        except PermissionError:
            self.status.set("Administrator access is required to write HKLM.")
        except OSError as error:
            self.status.set(f"Registry error: {error}")


def main():
    if sys.platform != "win32":
        raise SystemExit("This GUI is intended to run on Windows.")

    if not is_elevated():
        try:
            relaunch_as_admin()
        except OSError as error:
            messagebox.showerror("VDXR Settings", str(error))
        return

    root = tk.Tk()
    VDXRRegistryApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()