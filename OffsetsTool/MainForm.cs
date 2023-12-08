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
                aimPosX.Value = (int)key.GetValue("aim_pose_offset_x", 0);
                aimPosY.Value = (int)key.GetValue("aim_pose_offset_y", 0);
                aimPosZ.Value = (int)key.GetValue("aim_pose_offset_z", 0);
                aimRotX.Value = (int)key.GetValue("aim_pose_rot_x", 0);
                aimRotY.Value = (int)key.GetValue("aim_pose_rot_y", 0);
                aimRotZ.Value = (int)key.GetValue("aim_pose_rot_z", 0);
                gripPosX.Value = (int)key.GetValue("grip_pose_offset_x", 0);
                gripPosY.Value = (int)key.GetValue("grip_pose_offset_y", 0);
                gripPosZ.Value = (int)key.GetValue("grip_pose_offset_z", 0);
                gripRotX.Value = (int)key.GetValue("grip_pose_rot_x", 0);
                gripRotY.Value = (int)key.GetValue("grip_pose_rot_y", 0);
                gripRotZ.Value = (int)key.GetValue("grip_pose_rot_z", 0);
                palmPosX.Value = (int)key.GetValue("palm_pose_offset_x", 0);
                palmPosY.Value = (int)key.GetValue("palm_pose_offset_y", 0);
                palmPosZ.Value = (int)key.GetValue("palm_pose_offset_z", 0);
                palmRotX.Value = (int)key.GetValue("palm_pose_rot_x", 0);
                palmRotY.Value = (int)key.GetValue("palm_pose_rot_y", 0);
                palmRotZ.Value = (int)key.GetValue("palm_pose_rot_z", 0);
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
                key.DeleteValue("aim_pose_offset_x", false);
                key.DeleteValue("aim_pose_offset_y", false);
                key.DeleteValue("aim_pose_offset_z", false);
                key.DeleteValue("aim_pose_rot_x", false);
                key.DeleteValue("aim_pose_rot_y", false);
                key.DeleteValue("aim_pose_rot_z", false);
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
                key.DeleteValue("grip_pose_offset_x", false);
                key.DeleteValue("grip_pose_offset_y", false);
                key.DeleteValue("grip_pose_offset_z", false);
                key.DeleteValue("grip_pose_rot_x", false);
                key.DeleteValue("grip_pose_rot_y", false);
                key.DeleteValue("grip_pose_rot_z", false);
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
                key.DeleteValue("palm_pose_offset_x", false);
                key.DeleteValue("palm_pose_offset_y", false);
                key.DeleteValue("palm_pose_offset_z", false);
                key.DeleteValue("palm_pose_rot_x", false);
                key.DeleteValue("palm_pose_rot_y", false);
                key.DeleteValue("palm_pose_rot_z", false);
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
