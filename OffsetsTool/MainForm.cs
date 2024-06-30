using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Xml.Linq;

namespace OffsetsTool
{
    public partial class MainForm : Form
    {
        // Must match runtime.h.
        private string RegPrefix = "SOFTWARE\\Virtual Desktop, Inc.\\OpenXR";

        private bool m_isLoading = true;

        public MainForm()
        {
            InitializeComponent();

            LoadSettings();
        }

        private void LoadSettings()
        {
            m_isLoading = true;

            SuspendLayout();

            Microsoft.Win32.RegistryKey key = null;

            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(RegPrefix);
                aimPosX.Value = (int)key.GetValue("AimPoseOffsetX", 0);
                aimPosY.Value = (int)key.GetValue("AimPoseOffsetY", 0);
                aimPosZ.Value = (int)key.GetValue("AimPoseOffsetZ", 0);
                aimRotX.Value = (int)key.GetValue("AimPoseRotX", 0);
                aimRotY.Value = (int)key.GetValue("AimPoseRotY", 0);
                aimRotZ.Value = (int)key.GetValue("AimPoseRotZ", 0);
                gripPosX.Value = (int)key.GetValue("GripPoseOffsetX", 0);
                gripPosY.Value = (int)key.GetValue("GripPoseOffsetY", 0);
                gripPosZ.Value = (int)key.GetValue("GripPoseOffsetZ", 0);
                gripRotX.Value = (int)key.GetValue("GripPoseRotX", 0);
                gripRotY.Value = (int)key.GetValue("GripPoseRotY", 0);
                gripRotZ.Value = (int)key.GetValue("GripPoseRotZ", 0);
                palmPosX.Value = (int)key.GetValue("PalmPoseOffsetX", 0);
                palmPosY.Value = (int)key.GetValue("PalmPoseOffsetY", 0);
                palmPosZ.Value = (int)key.GetValue("PalmPoseOffsetZ", 0);
                palmRotX.Value = (int)key.GetValue("PalmPoseRotX", 0);
                palmRotY.Value = (int)key.GetValue("PalmPoseRotY", 0);
                palmRotZ.Value = (int)key.GetValue("PalmPoseRotZ", 0);
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

            ScrollEvent(aimPosX, null);
            ScrollEvent(aimPosY, null);
            ScrollEvent(aimPosZ, null);
            ScrollEvent(aimRotX, null);
            ScrollEvent(aimRotY, null);
            ScrollEvent(aimRotZ, null);
            ScrollEvent(gripPosX, null);
            ScrollEvent(gripPosY, null);
            ScrollEvent(gripPosZ, null);
            ScrollEvent(gripRotX, null);
            ScrollEvent(gripRotY, null);
            ScrollEvent(gripRotZ, null);
            ScrollEvent(palmPosX, null);
            ScrollEvent(palmPosY, null);
            ScrollEvent(palmPosZ, null);
            ScrollEvent(palmRotX, null);
            ScrollEvent(palmRotY, null);
            ScrollEvent(palmRotZ, null);

            ResumeLayout();

            m_isLoading = false;
        }

        private void ScrollEvent(object sender, EventArgs e)
        {
            TrackBar scroll = (TrackBar)sender;
            TextBox label = (TextBox)Controls.Find(scroll.Name + "Label", true)[0];
            label.Text = scroll.Value.ToString();

            if (m_isLoading)
            {
                return;
            }

            Microsoft.Win32.RegistryKey key = null;
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(RegPrefix);
                key.SetValue((string)scroll.Tag, scroll.Value, Microsoft.Win32.RegistryValueKind.DWord);
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

        private void restoreAim_Click(object sender, EventArgs e)
        {
            Microsoft.Win32.RegistryKey key = null;
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(RegPrefix);
                key.DeleteValue("AimPoseOffsetX", false);
                key.DeleteValue("AimPoseOffsetY", false);
                key.DeleteValue("AimPoseOffsetZ", false);
                key.DeleteValue("AimPoseRotX", false);
                key.DeleteValue("AimPoseRotY", false);
                key.DeleteValue("AimPoseRotZ", false);
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

        private void restoreGrip_Click(object sender, EventArgs e)
        {
            Microsoft.Win32.RegistryKey key = null;
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(RegPrefix);
                key.DeleteValue("GripPoseOffsetX", false);
                key.DeleteValue("GripPoseOffsetY", false);
                key.DeleteValue("GripPoseOffsetZ", false);
                key.DeleteValue("GripPoseRotX", false);
                key.DeleteValue("GripPoseRotY", false);
                key.DeleteValue("GripPoseRotZ", false);
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

        private void restorePalm_Click(object sender, EventArgs e)
        {
            Microsoft.Win32.RegistryKey key = null;
            try
            {
                key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(RegPrefix);
                key.DeleteValue("PalmPoseOffsetX", false);
                key.DeleteValue("PalmPoseOffsetY", false);
                key.DeleteValue("PalmPoseOffsetZ", false);
                key.DeleteValue("PalmPoseRotX", false);
                key.DeleteValue("PalmPoseRotY", false);
                key.DeleteValue("PalmPoseRotZ", false);
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
