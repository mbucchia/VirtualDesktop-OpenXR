// MIT License
//
// Copyright(c) 2022 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace companion
{
    public partial class MainForm : Form
    {
        [DllImport("pimax-openxr.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr getVersionString();

        // Must match runtime.h.
        public static readonly string RegPrefix = "SOFTWARE\\PimaxXR";

        private bool loading = true;
        private string pimaxRuntimePath = "";
        private string pimax32RuntimePath = "";
        private string steamRuntimePath = "";

        public MainForm()
        {
            InitializeComponent();
            GetPimaxXRVersion();

            SuspendLayout();

            Microsoft.Win32.RegistryKey key = null;

            // Try to locate SteamVR.
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey("SOFTWARE\\WOW6432Node\\Valve\\Steam");
                steamRuntimePath = (string)key.GetValue("InstallPath", "");
                if (steamRuntimePath != "")
                {
                    steamRuntimePath += "\\steamapps\\common\\SteamVR\\steamxr_win64.json";
                    if (!File.Exists(steamRuntimePath))
                    {
                        steamRuntimePath = "";
                    }
                }
            }
            catch (Exception)
            {
                steamRuntimePath = "";
            }
            finally
            {
                if (key != null)
                {
                    key.Close();
                }
            }

            if (steamRuntimePath == "")
            {
                MessageBox.Show(this, "Failed to locate SteamVR. Switching to the SteamVR runtime will not be offered.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                runtimeSteam.Enabled = false;
            }

            // Locate our own runtime.
            var assembly = Assembly.GetAssembly(GetType());
            var installPath = Path.GetDirectoryName(assembly.Location);
            pimaxRuntimePath = installPath + "\\pimax-openxr.json";
            pimax32RuntimePath = installPath + "\\pimax-openxr-32.json";

            // Read the OpenXR Loader configuration.
            var activeRuntime = "";
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey("SOFTWARE\\Khronos\\OpenXR\\1");
                activeRuntime = (string)key.GetValue("ActiveRuntime", "");
            }
            catch (Exception)
            {
            }
            finally
            {
                if (key != null)
                {
                    key.Close();
                }
            }

            if (activeRuntime == pimaxRuntimePath)
            {
                runtimePimax.Checked = true;
            }
            else if (steamRuntimePath != "" && activeRuntime == steamRuntimePath)
            {
                runtimeSteam.Checked = true;
            }
            else
            {
                MessageBox.Show(this, "Unable to identify the active OpenXR runtime: " + activeRuntime, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            // Read the PimaxXR configuration.
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(RegPrefix);

                // Must match the defaults in the runtime!
                recenterMode.Checked = (int)key.GetValue("recenter_on_startup", 1) == 1 ? true : false;
                joystickDeadzone.Value = (int)key.GetValue("joystick_deadzone", 2);
                enableTelemetry.Checked = (int)key.GetValue("enable_telemetry", 1) == 1 ? true : false;
            }
            catch (Exception)
            {
                MessageBox.Show(this, "Failed to write to registry. Please make sure the app is running elevated.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                if (key != null)
                {
                    key.Close();
                }
            }

            RefreshEnabledState();
            joystickDeadzone_Scroll(null, null);

            ResumeLayout();

            loading = false;
        }

        private unsafe void GetPimaxXRVersion()
        {
            try
            {
                IntPtr pStr = getVersionString();
                versionString.Text = Marshal.PtrToStringAnsi(pStr);
            }
            catch (Exception)
            {
                MessageBox.Show(this, "Failed to query version", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void SelectRuntime(string runtimePath, bool wow6432node = false)
        {
            Microsoft.Win32.RegistryKey key = null;
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(wow6432node ? "SOFTWARE\\WOW6432Node\\Khronos\\OpenXR\\1" : "SOFTWARE\\Khronos\\OpenXR\\1");
                if (runtimePath != null)
                {
                    key.SetValue("ActiveRuntime", runtimePath, Microsoft.Win32.RegistryValueKind.String);
                }
                else
                {
                    key.DeleteValue("ActiveRuntime", false);
                }
            }
            catch (Exception)
            {
                MessageBox.Show(this, "Failed to write to registry. Please make sure the app is running elevated.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                if (key != null)
                {
                    key.Close();
                }
            }
        }

        public static void WriteSetting(string name, int value)
        {
            Microsoft.Win32.RegistryKey key = null;
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(RegPrefix);
                key.SetValue(name, value, Microsoft.Win32.RegistryValueKind.DWord);
            }
            catch (Exception)
            {
                MessageBox.Show("Failed to write to registry. Please make sure the app is running elevated.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                if (key != null)
                {
                    key.Close();
                }
            }
        }

        private void RefreshEnabledState()
        {
            recenterMode.Enabled = joystickDeadzone.Enabled = joystickDeadzoneValue.Enabled = enableTelemetry.Enabled = 
                pitoolLabel.Enabled = joystickLabel.Enabled = telemetryLabel.Enabled = runtimePimax.Checked;
        }

        private void runtimePimax_CheckedChanged(object sender, EventArgs e)
        {
            if (runtimePimax.Checked)
            {
                SelectRuntime(pimaxRuntimePath);
                SelectRuntime(pimax32RuntimePath, true /* 32-bit */);
                RefreshEnabledState();
            }
        }

        private void runtimeSteam_CheckedChanged(object sender, EventArgs e)
        {
            if (runtimeSteam.Checked)
            {
                SelectRuntime(steamRuntimePath);
                SelectRuntime(null, true /* 32-bit */);
                RefreshEnabledState();
            }
        }

        private void recenterMode_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            WriteSetting("recenter_on_startup", recenterMode.Checked ? 1 : 0);
        }

        private void joystickDeadzone_Scroll(object sender, EventArgs e)
        {
            joystickDeadzoneValue.Text = joystickDeadzone.Value > 0 ? (joystickDeadzone.Value / 100.0f).ToString("#.##") : "0";

            if (loading)
            {
                return;
            }

            WriteSetting("joystick_deadzone", joystickDeadzone.Value);
        }

        private void enableTelemetry_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            WriteSetting("enable_telemetry", enableTelemetry.Checked ? 1 : 0);
        }

        private void openLogs_Click(object sender, EventArgs e)
        {
            var processInfo = new ProcessStartInfo();
            processInfo.Verb = "Open";
            processInfo.UseShellExecute = true;
            processInfo.FileName = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + "\\pimax-openxr\\pimax-openxr.log";
            try
            {
                Process.Start(processInfo);
            }
            catch (Win32Exception)
            {
                MessageBox.Show("Failed to open the log file. Please check attempt to locate '" + processInfo.FileName + "' manually.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void startTrace_Click(object sender, EventArgs e)
        {
            if (MessageBox.Show(this, "PRIVACY WARNING: The trace file generated by this tool may include the following personal information \"Name of the computer\", \"Windows account name\". By continuing, you consent to have this information collected and potentially exposed online (if you share the resulting file online)." +
                    "\n\nNOTE: Do not use this functionality unless instructed by the developers and you understand what you are doing.\n\nDo you wish to continue?", "Confirmation", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.No)
            {
                return;
            }

            // Cancel any pending traces.
            var processInfo = new ProcessStartInfo();
            processInfo.Verb = "Open";
            processInfo.FileName = "wpr";
            processInfo.Arguments = "-cancel";
            processInfo.CreateNoWindow = true;
            try
            {
                Process.Start(processInfo);
            }
            catch (Win32Exception)
            {
            }

            var assembly = Assembly.GetAssembly(GetType());
            var installPath = Path.GetDirectoryName(assembly.Location);

            // Start a new trace.
            processInfo = new ProcessStartInfo();
            processInfo.Verb = "Open";
            processInfo.FileName = "wpr";
            processInfo.Arguments = "-start \"" + installPath + "\\PimaxOpenXR.wprp\" -filemode";
            processInfo.CreateNoWindow = true;
            processInfo.RedirectStandardOutput = true;
            processInfo.RedirectStandardError = true;
            processInfo.UseShellExecute = false;
            try
            {
                var process = Process.Start(processInfo);
                process.WaitForExit();

                var output = process.StandardOutput.ReadToEnd();
                var error = process.StandardError.ReadToEnd();
                if (output != "" || error != "")
                {
                    MessageBox.Show(this, "Standard output:\n" + output + "\nStandard error:\n" + error, "Outcome", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }
            catch (Win32Exception)
            {
                MessageBox.Show(this, "Failed to start tracing. Please make sure the app is running elevated.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            startTrace.Enabled = false;
            stopTrace.Enabled = true;

            MessageBox.Show(this, "Do not close the Control Center app until after you have stopped the capture.", "Confirmation", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void stopTrace_Click(object sender, EventArgs e)
        {
            var processInfo = new ProcessStartInfo();
            processInfo.Verb = "Open";
            processInfo.FileName = "wpr";
            processInfo.Arguments = "-stop \"" + Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + "\\pimax-openxr\\PimaxXR.etl\"";
            processInfo.CreateNoWindow = true;
            processInfo.RedirectStandardOutput = true;
            processInfo.RedirectStandardError = true;
            processInfo.UseShellExecute = false;
            try
            {
                var process = Process.Start(processInfo);
                process.WaitForExit();

                var output = process.StandardOutput.ReadToEnd();
                var error = process.StandardError.ReadToEnd();
                if (output != "" || error != "")
                {
                    MessageBox.Show(this, "Standard output:\n" + output + "\nStandard error:\n" + error, "Outcome", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }
            catch (Win32Exception)
            {
                MessageBox.Show(this, "Failed to save trace. Please make sure the app is running elevated.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            startTrace.Enabled = true;
            stopTrace.Enabled = false;

            // Open the output folder with our file.
            processInfo = new ProcessStartInfo();
            processInfo.Verb = "Open";
            processInfo.UseShellExecute = true;
            processInfo.FileName = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + "\\pimax-openxr";
            try
            {
                Process.Start(processInfo);
            }
            catch (Win32Exception)
            {
            }
        }

        private void gotoDownloads_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            string githubReleases = "https://github.com/mbucchia/Pimax-OpenXR/releases";

            gotoDownloads.LinkVisited = true;
            System.Diagnostics.Process.Start(githubReleases);
        }

        private void reportIssues_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            string githubIssues = "https://github.com/mbucchia/Pimax-OpenXR/issues?q=is%3Aissue+is%3Aopen";

            reportIssues.LinkVisited = true;
            System.Diagnostics.Process.Start(githubIssues);
        }

        private ExperimentalSettings experimentalSettings = new ExperimentalSettings();
        private uint secretHandshake = 0;

        private void pictureBox1_DoubleClick(object sender, EventArgs e)
        {
            secretHandshake++;
            if (secretHandshake >= 5)
            {
                experimentalSettings.Show();
                secretHandshake = 0;
            }
        }
    }
}
