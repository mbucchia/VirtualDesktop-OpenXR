// MIT License
//
// Copyright(c) 2022-2023 Matthieu Bucchianeri
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

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        unsafe struct RuntimeStatus
        {
            public bool valid;

            public float refreshRate;
            public uint resolutionWidth;
            public uint resolutionHeight;
            public byte fovLevel;
            public float fov;
            public float floorHeight;
            public bool useParallelProjection;
            public bool useSmartSmoothing;
            public bool useLighthouseTracking;
            public float fps;
        }

        [DllImport("pimax-openxr.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void getRuntimeStatus(IntPtr status);

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
                swapGripAimPoses.Checked = (int)key.GetValue("swap_grip_aim_poses", 0) == 1 ? true : false;
                controllerEmulation.SelectedIndex = (int)key.GetValue("force_interaction_profile", 0);
                joystickDeadzone.Value = (int)key.GetValue("joystick_deadzone", 2);
                guardian.Checked = (int)key.GetValue("guardian", 1) == 1 ? true : false;
                guardianRadius.Value = (int)key.GetValue("guardian_radius", 1600) / 10;
                guardianThreshold.Value = (int)key.GetValue("guardian_threshold", 1100) / 10;
                mirrorMode.Checked = (int)key.GetValue("mirror_window", 0) == 1 ? true : false;
                enableTelemetry.Checked = (int)key.GetValue("enable_telemetry", 0) == 1 ? true : false;
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
            guardianRadius_Scroll(null, null);
            guardianThreshold_Scroll(null, null);

            ResumeLayout();

            GetRuntimeStatus();

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

        private unsafe void GetRuntimeStatus()
        {
            try
            {
                var fov = new string[] { "Large", "Normal", "Small", "Potato" };
                RuntimeStatus status = new RuntimeStatus();
                status.valid = false;

                getRuntimeStatus(new IntPtr(&status));
                var label = "Resolution: " + status.resolutionWidth + "x" + status.resolutionHeight + " @ " + status.refreshRate.ToString("#") + " Hz" +
                    "   Horizontal FOV: " + status.fov.ToString("#.#") + " deg (" + fov[status.fovLevel] + ")\n";
                if (status.useLighthouseTracking)
                {
                    label += "Lighthouse tracking, ";
                }
                if (status.useParallelProjection)
                {
                    label += "Parallel projections, ";
                }
                if (status.useSmartSmoothing)
                {
                    label += "Smart Smoothing, ";
                }
                label += "Floor height: " + status.floorHeight.ToString("#.##") + " m";
                runtimeStatusLabel.Text = label;
            }
            catch (Exception)
            {
                runtimeStatusLabel.Text = "Pimax runtime status is not available.";
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
            runtimeStatusLabel.Enabled = recenterMode.Enabled = recenterLabel.Enabled = swapGripAimPoses.Enabled = controllerEmulation.Enabled = controllerEmulationLabel.Enabled =
                joystickDeadzone.Enabled = joystickDeadzoneValue.Enabled = joystickLabel.Enabled = guardian.Enabled = mirrorMode.Enabled = enableTelemetry.Enabled = pitoolLabel.Enabled =
                telemetryLabel.Enabled = runtimePimax.Checked;
            guardianLabel1.Enabled = guardianLabel2.Enabled = guardianRadius.Enabled = guardianRadiusValue.Enabled = guardianThreshold.Enabled = guardianThresholdValue.Enabled = guardian.Enabled && guardian.Checked;
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

        private void swapGripAimPoses_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            WriteSetting("swap_grip_aim_poses", swapGripAimPoses.Checked ? 1 : 0);
        }

        private void controllerEmulation_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            WriteSetting("force_interaction_profile", controllerEmulation.SelectedIndex);
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

        private void guardian_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            WriteSetting("guardian", guardian.Checked ? 1 : 0);
            RefreshEnabledState();
        }

        private void guardianRadius_Scroll(object sender, EventArgs e)
        {
            guardianRadiusValue.Text = guardianRadius.Value > 0 ? (guardianRadius.Value / 100.0f).ToString("#.##") : "0";

            if (loading)
            {
                return;
            }

            WriteSetting("guardian_radius", guardianRadius.Value * 10);

            if (guardianThreshold.Value > guardianRadius.Value)
            {
                guardianThreshold.Value = guardianRadius.Value;
                guardianThreshold_Scroll(null, null);
            }
        }

        private void guardianThreshold_Scroll(object sender, EventArgs e)
        {
            guardianThresholdValue.Text = guardianThreshold.Value > 0 ? (guardianThreshold.Value / 100.0f).ToString("#.##") : "0";

            if (loading)
            {
                return;
            }

            WriteSetting("guardian_threshold", guardianThreshold.Value * 10);

            if (guardianRadius.Value < guardianThreshold.Value)
            {
                guardianRadius.Value = guardianThreshold.Value;
                guardianRadius_Scroll(null, null);
            }
        }

        private void mirrorMode_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            WriteSetting("mirror_window", mirrorMode.Checked ? 1 : 0);
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

        private void runtimeStatusLabel_Click(object sender, EventArgs e)
        {
            GetRuntimeStatus();
        }
    }
}
