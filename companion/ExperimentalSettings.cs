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
                droolonProjectionDistance.Value = (int)key.GetValue("droolon_projection_distance", 35);
                enableQuadViews.Checked = (int)key.GetValue("disable_quad_views", 1) == 0 ? true : false;
                focusDensity.Value = (int)key.GetValue("focus_density", 1000);
                peripheralDensity.Value = (int)key.GetValue("peripheral_density", 500);
                horizontalSection1.Value = (int)key.GetValue("focus_horizontal_section", 750);
                horizontalSection2.Value = (int)key.GetValue("focus_horizontal_section_foveated", 500);
                verticalSection1.Value = (int)key.GetValue("focus_vertical_section", 700);
                verticalSection2.Value = (int)key.GetValue("focus_vertical_section_foveated", 500);
                preferFoveated.Checked = (int)key.GetValue("prefer_foveated_rendering", 1) == 1 ? true : false;

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
            droolonProjectionDistance_Scroll(null, null);
            focusDensity_Scroll(null, null);
            peripheralDensity_Scroll(null, null);
            horizontalSection1_Scroll(null, null);
            horizontalSection2_Scroll(null, null);
            verticalSection1_Scroll(null, null);
            verticalSection2_Scroll(null, null);

            ResumeLayout();
            loading = false;
        }

        private void RefreshEnabledState()
        {
            forceRateLabel.Enabled = forceHalf.Enabled = forceThird.Enabled = enableFrameTiming.Checked;
            filterLength.Enabled = filterLengthLabel.Enabled = filterLengthValue.Enabled =
                timingBias.Enabled = timingBiasLabel.Enabled = timingBiasValue.Enabled = enableFrameTiming.Checked && !(forceHalf.Checked || forceThird.Checked);
            focusDensity.Enabled = focusDensityLabel.Enabled = focusDensityValue.Enabled =
                peripheralDensity.Enabled = peripheralDensityLabel.Enabled = peripheralDensityValue.Enabled =
                horizontalSection1.Enabled = horizontalSection1Label.Enabled = horizontalSection1Value.Enabled =
                horizontalSection2.Enabled = horizontalSection2Label.Enabled = horizontalSection2Value.Enabled =
                verticalSection1.Enabled = verticalSection1Label.Enabled = verticalSection1Value.Enabled =
                verticalSection2.Enabled = verticalSection2Label.Enabled = verticalSection2Value.Enabled =
                preferFoveated.Enabled = enableQuadViews.Checked;
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

        private void droolonProjectionDistance_Scroll(object sender, EventArgs e)
        {
            droolonProjectionDistanceValue.Text = droolonProjectionDistance.Value != 0 ? (droolonProjectionDistance.Value / 100.0f).ToString("#.##") : "0";

            if (loading)
            {
                return;
            }

            MainForm.WriteSetting("droolon_projection_distance", droolonProjectionDistance.Value);
        }

        private void enableQuadViews_CheckedChanged(object sender, EventArgs e)
        {
            RefreshEnabledState();

            if (loading)
            {
                return;
            }

            MainForm.WriteSetting("disable_quad_views", enableQuadViews.Checked ? 0 : 1);
        }

        private void focusDensity_Scroll(object sender, EventArgs e)
        {
            // Use the input in tenth of percent to allow one decimal.
            focusDensityValue.Text = (focusDensity.Value / 10.0f).ToString("#.#");

            if (loading)
            {
                return;
            }

            // Store in tenth of percent.
            MainForm.WriteSetting("focus_density", focusDensity.Value);
        }

        private void peripheralDensity_Scroll(object sender, EventArgs e)
        {
            // Use the input in tenth of percent to allow one decimal.
            peripheralDensityValue.Text = (peripheralDensity.Value / 10.0f).ToString("#.#");

            if (loading)
            {
                return;
            }

            // Store in tenth of percent.
            MainForm.WriteSetting("peripheral_density", peripheralDensity.Value);
        }

        private void horizontalSection1_Scroll(object sender, EventArgs e)
        {
            // Use the input in tenth of percent to allow one decimal.
            horizontalSection1Value.Text = (horizontalSection1.Value / 10.0f).ToString("#.#");

            if (loading)
            {
                return;
            }

            // Store in tenth of percent.
            MainForm.WriteSetting("focus_horizontal_section", horizontalSection1.Value);
        }

        private void horizontalSection2_Scroll(object sender, EventArgs e)
        {
            // Use the input in tenth of percent to allow one decimal.
            horizontalSection2Value.Text = (horizontalSection2.Value / 10.0f).ToString("#.#");

            if (loading)
            {
                return;
            }

            // Store in tenth of percent.
            MainForm.WriteSetting("focus_horizontal_section_foveated", horizontalSection2.Value);
        }

        private void verticalSection1_Scroll(object sender, EventArgs e)
        {
            // Use the input in tenth of percent to allow one decimal.
            verticalSection1Value.Text = (verticalSection1.Value / 10.0f).ToString("#.#");

            if (loading)
            {
                return;
            }

            // Store in tenth of percent.
            MainForm.WriteSetting("focus_vertical_section", verticalSection1.Value);
        }

        private void verticalSection2_Scroll(object sender, EventArgs e)
        {
            // Use the input in tenth of percent to allow one decimal.
            verticalSection2Value.Text = (verticalSection2.Value / 10.0f).ToString("#.#");

            if (loading)
            {
                return;
            }

            // Store in tenth of percent.
            MainForm.WriteSetting("focus_vertical_section_foveated", verticalSection2.Value);
        }

        private void preferFoveated_CheckedChanged(object sender, EventArgs e)
        {
            if (loading)
            {
                return;
            }

            MainForm.WriteSetting("prefer_foveated_rendering", preferFoveated.Checked ? 1 : 0);
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
                key.DeleteValue("droolon_projection_distance", false);
                key.DeleteValue("disable_quad_views", false);
                key.DeleteValue("focus_density", false);
                key.DeleteValue("peripheral_density", false);
                key.DeleteValue("focus_horizontal_section", false);
                key.DeleteValue("focus_horizontal_section_foveated", false);
                key.DeleteValue("focus_vertical_section", false);
                key.DeleteValue("focus_vertical_section_foveated", false);
                key.DeleteValue("prefer_foveated_rendering", false);
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
