using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Xml.Linq;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;
using TextBox = System.Windows.Forms.TextBox;
using TrackBar = System.Windows.Forms.TrackBar;

namespace AdvancedSettings
{
    public partial class MainForm : Form
    {
        private Microsoft.Win32.RegistryKey Key;

        private string RegPrefix;
        private string RuntimeName;
        private bool IsLoading = true;

        public MainForm(string regPrefix, string runtimeName)
        {
            RegPrefix = regPrefix;
            RuntimeName = runtimeName;

            LoadSettings();
        }

        public void LoadSettings()
        {
            IsLoading = true;

            SuspendLayout();

            while (Controls.Count > 0)
            {
                Controls[0].Dispose();
            }
            InitializeComponent();

            Text = RuntimeName + " " + Text;

            try
            {
                Key = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(RegPrefix, true);
                foreach (var control in this.Descendants<Control>())
                {
                    var trackbar = control as TrackBar;
                    if (trackbar != null && trackbar.Tag != null)
                    {
                        trackbar.Value = Math.Min(Math.Max((int)Key.GetValue((string)trackbar.Tag, (int)trackbar.Value), trackbar.Minimum), trackbar.Maximum);
                        ScrollEvent(trackbar, null);
                    }
                }
                foreach (var control in this.Descendants<CheckBox>())
                {
                    var checkbox = control as CheckBox;
                    if (checkbox != null && checkbox.Tag != null)
                    {
                        checkbox.Checked = (int)Key.GetValue((string)checkbox.Tag, checkbox.Checked ? 1 : 0) != 0;
                    }
                }
                playerHeightEnable.Checked = (int)Key.GetValue("OverrideFloorHeight", -1) > 0;
                playerHeightEnable_CheckedChanged(playerHeightEnable, null);
                playerHeight_Scroll(playerHeight, null);
                quadviewsEnable_CheckedChanged(quadviewsEnable, null);
            }
            catch (Exception)
            {
                MessageBox.Show(this, "Failed to write to registry. Please make sure the app is running elevated.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            ResumeLayout();

            IsLoading = false;
        }

        private void ScrollEvent(object sender, EventArgs e)
        {
            TrackBar scroll = (TrackBar)sender;
            TextBox label = (TextBox)Controls.Find(scroll.Name + "Value", true)[0];
            label.Text = scroll.Value.ToString();

            if (IsLoading || Key == null)
            {
                return;
            }

            Key.SetValue((string)scroll.Tag, scroll.Value, Microsoft.Win32.RegistryValueKind.DWord);
        }

        private void restoreDefaults_Click(object sender, EventArgs e)
        {
            if (Key == null)
            {
                return;
            }

            foreach (var control in this.Descendants<Control>())
            {
                var trackbar = control as TrackBar;
                if (trackbar != null && trackbar.Tag != null)
                {
                    Key.DeleteValue((string)trackbar.Tag, false);
                }
            }
            foreach (var control in this.Descendants<CheckBox>())
            {
                var checkbox = control as CheckBox;
                if (checkbox != null && checkbox.Tag != null)
                {
                    Key.DeleteValue((string)checkbox.Tag, false);
                }
            }

            LoadSettings();
        }

        private void CheckedChanged(object sender, EventArgs e)
        {
            CheckBox checkbox = (CheckBox)sender;

            if (IsLoading || Key == null)
            {
                return;
            }

            Key.SetValue((string)checkbox.Tag, checkbox.Checked ? 1 : 0, Microsoft.Win32.RegistryValueKind.DWord);
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            string githubWiki = "https://github.com/mbucchia/VirtualDesktop-OpenXR/wiki/Advanced-Settings";

            linkLabel1.LinkVisited = true;
            System.Diagnostics.Process.Start(githubWiki);

        }

        private void playerHeightEnable_CheckedChanged(object sender, EventArgs e)
        {
            playerHeight.Enabled = playerHeightValue.Enabled = label17.Enabled = playerHeightValueFt.Enabled = label23.Enabled =
                playerHeightEnable.Checked;

            if (IsLoading || Key == null)
            {
                return;
            }

            if (playerHeightEnable.Checked)
            {
                playerHeight.Value = (int)Key.GetValue("OverrideFloorHeight", 170);
                playerHeight_Scroll(playerHeight, null);
            }
            else
            {
                Key.SetValue("OverrideFloorHeight", -1, Microsoft.Win32.RegistryValueKind.DWord);
            }
        }

        private void playerHeight_Scroll(object sender, EventArgs e)
        {
            ScrollEvent(sender, e);
            int feet = (int)(playerHeight.Value / 30.48);
            double inches = (playerHeight.Value / 2.54);
            playerHeightValueFt.Text = (int)feet + "' " + (int)(inches % 12) + "''";
        }

        private void quadviewsEnable_CheckedChanged(object sender, EventArgs e)
        {
            label19.Enabled = peripheralView.Enabled = peripheralViewValue.Enabled = label20.Enabled =
                label21.Enabled = focusSize.Enabled = focusSizeValue.Enabled = label21.Enabled =
                quadviewsEnable.Checked;

            CheckedChanged(quadviewsEnable, null);
        }
    }

    public static class ControlExtensions
    {
        public static IEnumerable<T> Descendants<T>(this Control control) where T : class
        {
            foreach (Control child in control.Controls)
            {
                if (child is T thisControl)
                {
                    yield return (T)thisControl;
                }

                if (child.HasChildren)
                {
                    foreach (T descendant in Descendants<T>(child))
                    {
                        yield return descendant;
                    }
                }
            }
        }
    }
}
