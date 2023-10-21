using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows.Forms;

// Reference: https://www.c-sharpcorner.com/article/how-to-perform-custom-actions-and-upgrade-using-visual-studio-installer/
namespace SetupCustomActions
{
    [RunInstaller(true)]
    public partial class CustomActions : System.Configuration.Install.Installer
    {
        public CustomActions()
        {
        }

        private class WindowWrapper : IWin32Window
        {
            private readonly IntPtr hwnd;

            public IntPtr Handle
            {
                get { return hwnd; }
            }

            public WindowWrapper(IntPtr handle)
            {
                hwnd = handle;
            }
        }

        protected override void OnBeforeInstall(IDictionary savedState)
        {
            var installPath = Path.GetDirectoryName(base.Context.Parameters["AssemblyPath"]);

            // https://stackoverflow.com/questions/6213498/custom-installer-in-net-showing-form-behind-installer
            var proc = Process.GetProcessesByName("msiexec").FirstOrDefault(p => p.MainWindowTitle == "VirtualDesktopXR (Standalone)");
            var owner = proc != null ? new WindowWrapper(proc.MainWindowHandle) : null;

            Microsoft.Win32.RegistryKey key;
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey("SOFTWARE\\Khronos\\OpenXR\\1\\AvailableRuntimes");
                var existingValues = key.GetValueNames();

                foreach (var value in existingValues)
                {
                    if (value.EndsWith("\\Virtual Desktop Streamer\\OpenXR\\virtualdesktop-openxr.json"))
                    {
                        var result = MessageBox.Show(owner,
                            "Detected that the Virtual Desktop Streamer is installed.\n" +
                            "The Virtual Desktop Streamer ALREADY INCLUDES a stable version of VirtualDesktopXR.\n\n" +
                            "It is recommended to use the version of VirtualDesktopXR that is bundled with the Virtual Desktop Streamer.\n\n" +
                            "Are you sure you want to continue and install the standalone version of VirtualDesktopXR? (ADVANCED USERS)",
                            "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                        if (result == DialogResult.No)
                        {
                            throw new Exception("Installation was cancelled");
                        }
                    }
                }

                key.Close();
            }

            base.OnAfterInstall(savedState);
        }
    }
}
