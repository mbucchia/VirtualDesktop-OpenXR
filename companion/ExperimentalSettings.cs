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
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace companion
{
    public partial class ExperimentalSettings : Form
    {
        private bool loading = true;

        public ExperimentalSettings()
        {
            InitializeComponent();
            LoadSettings();
        }

        private void ExperimentalSettings_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                Hide();
            }
        }

        private void LoadSettings()
        {
            loading = true;
            SuspendLayout();

            Microsoft.Win32.RegistryKey key = null;

            // Read the PimaxXR configuration.
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(MainForm.RegPrefix);

                // Must match the defaults in the runtime!
                enableFrameTiming.Checked = (int)key.GetValue("use_frame_timing_override", 1) == 1 ? true : false;
                filterLength.Value = (int)key.GetValue("frame_time_filter_length", 5);
                var multiplier = (int)key.GetValue("frame_time_override_multiplier", 0);
                if (multiplier == 100)
                {
                    forceHalf.Checked = true;
                }
                else if (multiplier == 200)
                {
                    forceThird.Checked = true;
                }
                else
                {
                    forceHalf.Checked = forceThird.Checked = true;
                }
                // Convert value from microseconds to tenth of milliseconds.
                timingBias.Value = multiplier == 0 ? ((int)key.GetValue("frame_time_override_offset", 0) / 100) : 0;
                disableFramePipelining.Checked = (int)key.GetValue("quirk_disable_frame_pipelining", 0) == 1 ? true : false;
                alwaysUseFrameIdZero.Checked = (int)key.GetValue("quirk_always_use_frame_id_zero", 0) == 1 ? true : false;
                forceDisableParallelProjection.Checked = (int)key.GetValue("force_parallel_projection_state", 1) == 0 ? true : false;

                // DO NOT FORGET TO ADD TO restoreDefaults_Click()!
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
            filterLength_Scroll(null, null);
            timingBias_Scroll(null, null);

            ResumeLayout();

            loading = false;
        }

        private void RefreshEnabledState()
        {
            forceRateLabel.Enabled = forceHalf.Enabled = forceThird.Enabled = enableFrameTiming.Checked;
            filterLength.Enabled = filterLengthLabel.Enabled = filterLengthValue.Enabled =
                timingBias.Enabled = timingBiasLabel.Enabled = timingBiasValue.Enabled = enableFrameTiming.Checked && !(forceHalf.Checked || forceThird.Checked);
        }

        private void enableFrameTiming_CheckedChanged(object sender, EventArgs e)
        {
            RefreshEnabledState();

            if (loading)
            {
                return;
            }

            MainForm.WriteSetting("use_frame_timing_override", enableFrameTiming.Checked ? 1 : 0);
        }

        private void filterLength_Scroll(object sender, EventArgs e)
        {
            filterLengthValue.Text = filterLength.Value.ToString();

            if (loading)
            {
                return;
            }

            MainForm.WriteSetting("frame_time_filter_length", filterLength.Value);
        }

        private void timingBias_Scroll(object sender, EventArgs e)
        {
            // Use the input in tenth of milliseconds to allow one decimal.
            timingBiasValue.Text = timingBias.Value != 0 ? (timingBias.Value / 10.0f).ToString("#.##") : "0";

            if (loading)
            {
                return;
            }

            // Store in microseconds.
            MainForm.WriteSetting("frame_time_override_offset", timingBias.Value * 100);
        }

        private void forceHalf_CheckedChanged(object sender, EventArgs e)
        {
            forceThird.Checked = false;
            RefreshEnabledState();

            if (loading)
            {
                return;
            }

            if (forceHalf.Checked)
            {
                // Force 100% frame duration + 1ms.
                MainForm.WriteSetting("frame_time_override_multiplier", 100);
                MainForm.WriteSetting("frame_time_override_offset", 1000);
            }
            else
            {
                MainForm.WriteSetting("frame_time_override_multiplier", 0);
                timingBias_Scroll(null, null);
            }
        }

        private void forceThird_CheckedChanged(object sender, EventArgs e)
        {
            forceHalf.Checked = false;
            RefreshEnabledState();

            if (loading)
            {
                return;
            }

            if (forceThird.Checked)
            {
                // Force 200% frame duration + 1ms.
                MainForm.WriteSetting("frame_time_override_multiplier", 200);
                MainForm.WriteSetting("frame_time_override_offset", 1000);
            }
            else
            {
                MainForm.WriteSetting("frame_time_override_multiplier", 0);
                timingBias_Scroll(null, null);
            }
        }

        private void disableFramePipelining_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            MainForm.WriteSetting("quirk_disable_frame_pipelining", disableFramePipelining.Checked ? 1 : 0);
        }

        private void alwaysUseFrameIdZero_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            MainForm.WriteSetting("quirk_always_use_frame_id_zero", alwaysUseFrameIdZero.Checked ? 1 : 0);
        }

        private void forceDisableParallelProjection_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            if (forceDisableParallelProjection.Checked)
            {
                MainForm.WriteSetting("force_parallel_projection_state", 0);
            } else
            {
                Microsoft.Win32.RegistryKey key = null;
                try
                {
                    key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(MainForm.RegPrefix);
                    key.DeleteValue("force_parallel_projection_state", false);
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
            }
        }

        private void restoreDefaults_Click(object sender, EventArgs e)
        {
            Microsoft.Win32.RegistryKey key = null;

            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(MainForm.RegPrefix);

                key.DeleteValue("use_frame_timing_override", false);
                key.DeleteValue("frame_time_filter_length", false);
                key.DeleteValue("frame_time_override_multiplier", false);
                key.DeleteValue("frame_time_override_offset", false);
                key.DeleteValue("quirk_disable_frame_pipelining", false);
                key.DeleteValue("quirk_always_use_frame_id_zero", false);
                key.DeleteValue("force_parallel_projection_state", false);
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

            LoadSettings();
        }
    }
}
