
namespace companion
{
    partial class ExperimentalSettings
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ExperimentalSettings));
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.enableFrameTiming = new System.Windows.Forms.CheckBox();
            this.filterLengthLabel = new System.Windows.Forms.Label();
            this.filterLength = new System.Windows.Forms.TrackBar();
            this.timingBiasLabel = new System.Windows.Forms.Label();
            this.timingBias = new System.Windows.Forms.TrackBar();
            this.forceRateLabel = new System.Windows.Forms.Label();
            this.filterLengthValue = new System.Windows.Forms.TextBox();
            this.timingBiasValue = new System.Windows.Forms.TextBox();
            this.forceHalf = new System.Windows.Forms.CheckBox();
            this.forceThird = new System.Windows.Forms.CheckBox();
            this.restoreDefaults = new System.Windows.Forms.Button();
            this.flowLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.filterLength)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.timingBias)).BeginInit();
            this.SuspendLayout();
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.enableFrameTiming);
            this.flowLayoutPanel1.Controls.Add(this.filterLengthValue);
            this.flowLayoutPanel1.Controls.Add(this.filterLengthLabel);
            this.flowLayoutPanel1.Controls.Add(this.filterLength);
            this.flowLayoutPanel1.Controls.Add(this.timingBiasValue);
            this.flowLayoutPanel1.Controls.Add(this.timingBiasLabel);
            this.flowLayoutPanel1.Controls.Add(this.timingBias);
            this.flowLayoutPanel1.Controls.Add(this.forceRateLabel);
            this.flowLayoutPanel1.Controls.Add(this.forceHalf);
            this.flowLayoutPanel1.Controls.Add(this.forceThird);
            this.flowLayoutPanel1.Controls.Add(this.restoreDefaults);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(300, 194);
            this.flowLayoutPanel1.TabIndex = 0;
            // 
            // enableFrameTiming
            // 
            this.enableFrameTiming.AutoSize = true;
            this.flowLayoutPanel1.SetFlowBreak(this.enableFrameTiming, true);
            this.enableFrameTiming.Location = new System.Drawing.Point(3, 3);
            this.enableFrameTiming.Name = "enableFrameTiming";
            this.enableFrameTiming.Padding = new System.Windows.Forms.Padding(3, 6, 0, 0);
            this.enableFrameTiming.Size = new System.Drawing.Size(293, 23);
            this.enableFrameTiming.TabIndex = 0;
            this.enableFrameTiming.Text = "Enable Smart Smoothing timing control (Requires restart)";
            this.enableFrameTiming.UseVisualStyleBackColor = true;
            this.enableFrameTiming.CheckedChanged += new System.EventHandler(this.enableFrameTiming_CheckedChanged);
            // 
            // filterLengthLabel
            // 
            this.filterLengthLabel.AutoSize = true;
            this.filterLengthLabel.Location = new System.Drawing.Point(44, 29);
            this.filterLengthLabel.Name = "filterLengthLabel";
            this.filterLengthLabel.Padding = new System.Windows.Forms.Padding(0, 9, 0, 0);
            this.filterLengthLabel.Size = new System.Drawing.Size(128, 22);
            this.filterLengthLabel.TabIndex = 2;
            this.filterLengthLabel.Text = "Length of smoothing filter:";
            // 
            // filterLength
            // 
            this.flowLayoutPanel1.SetFlowBreak(this.filterLength, true);
            this.filterLength.LargeChange = 10;
            this.filterLength.Location = new System.Drawing.Point(178, 32);
            this.filterLength.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.filterLength.Maximum = 600;
            this.filterLength.Minimum = 3;
            this.filterLength.Name = "filterLength";
            this.filterLength.Size = new System.Drawing.Size(104, 45);
            this.filterLength.TabIndex = 3;
            this.filterLength.TickFrequency = 10;
            this.filterLength.Value = 5;
            this.filterLength.Scroll += new System.EventHandler(this.filterLength_Scroll);
            // 
            // timingBiasLabel
            // 
            this.timingBiasLabel.AutoSize = true;
            this.timingBiasLabel.Location = new System.Drawing.Point(44, 77);
            this.timingBiasLabel.Name = "timingBiasLabel";
            this.timingBiasLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
            this.timingBiasLabel.Size = new System.Drawing.Size(136, 19);
            this.timingBiasLabel.TabIndex = 5;
            this.timingBiasLabel.Text = "GPU Frame Time Bias (ms):";
            // 
            // timingBias
            // 
            this.flowLayoutPanel1.SetFlowBreak(this.timingBias, true);
            this.timingBias.Location = new System.Drawing.Point(186, 80);
            this.timingBias.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.timingBias.Maximum = 300;
            this.timingBias.Minimum = -300;
            this.timingBias.Name = "timingBias";
            this.timingBias.Size = new System.Drawing.Size(104, 45);
            this.timingBias.TabIndex = 6;
            this.timingBias.TickFrequency = 10;
            this.timingBias.Scroll += new System.EventHandler(this.timingBias_Scroll);
            // 
            // forceRateLabel
            // 
            this.forceRateLabel.AutoSize = true;
            this.forceRateLabel.Location = new System.Drawing.Point(2, 125);
            this.forceRateLabel.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.forceRateLabel.Name = "forceRateLabel";
            this.forceRateLabel.Padding = new System.Windows.Forms.Padding(3, 3, 0, 0);
            this.forceRateLabel.Size = new System.Drawing.Size(158, 16);
            this.forceRateLabel.TabIndex = 7;
            this.forceRateLabel.Text = "Force fractional smoothing rate:";
            // 
            // filterLengthValue
            // 
            this.filterLengthValue.Location = new System.Drawing.Point(6, 35);
            this.filterLengthValue.Margin = new System.Windows.Forms.Padding(6, 6, 3, 3);
            this.filterLengthValue.Name = "filterLengthValue";
            this.filterLengthValue.ReadOnly = true;
            this.filterLengthValue.Size = new System.Drawing.Size(32, 20);
            this.filterLengthValue.TabIndex = 1;
            // 
            // timingBiasValue
            // 
            this.timingBiasValue.Location = new System.Drawing.Point(6, 80);
            this.timingBiasValue.Margin = new System.Windows.Forms.Padding(6, 3, 3, 3);
            this.timingBiasValue.Name = "timingBiasValue";
            this.timingBiasValue.ReadOnly = true;
            this.timingBiasValue.Size = new System.Drawing.Size(32, 20);
            this.timingBiasValue.TabIndex = 4;
            // 
            // forceHalf
            // 
            this.forceHalf.AutoSize = true;
            this.forceHalf.Location = new System.Drawing.Point(165, 128);
            this.forceHalf.Name = "forceHalf";
            this.forceHalf.Size = new System.Drawing.Size(43, 17);
            this.forceHalf.TabIndex = 8;
            this.forceHalf.Text = "1/2";
            this.forceHalf.UseVisualStyleBackColor = true;
            this.forceHalf.CheckedChanged += new System.EventHandler(this.forceHalf_CheckedChanged);
            // 
            // forceThird
            // 
            this.forceThird.AutoSize = true;
            this.forceThird.Location = new System.Drawing.Point(214, 128);
            this.forceThird.Name = "forceThird";
            this.forceThird.Size = new System.Drawing.Size(43, 17);
            this.forceThird.TabIndex = 9;
            this.forceThird.Text = "1/3";
            this.forceThird.UseVisualStyleBackColor = true;
            this.forceThird.CheckedChanged += new System.EventHandler(this.forceThird_CheckedChanged);
            // 
            // restoreDefaults
            // 
            this.restoreDefaults.Location = new System.Drawing.Point(6, 151);
            this.restoreDefaults.Margin = new System.Windows.Forms.Padding(6, 3, 3, 3);
            this.restoreDefaults.Name = "restoreDefaults";
            this.restoreDefaults.Size = new System.Drawing.Size(126, 39);
            this.restoreDefaults.TabIndex = 10;
            this.restoreDefaults.Text = "Restore defaults";
            this.restoreDefaults.UseVisualStyleBackColor = true;
            this.restoreDefaults.Click += new System.EventHandler(this.restoreDefaults_Click);
            // 
            // ExperimentalSettings
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(300, 194);
            this.Controls.Add(this.flowLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ExperimentalSettings";
            this.Text = "PimaxXR - Experimental Settings";
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.filterLength)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.timingBias)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.CheckBox enableFrameTiming;
        private System.Windows.Forms.Label filterLengthLabel;
        private System.Windows.Forms.TrackBar filterLength;
        private System.Windows.Forms.Label timingBiasLabel;
        private System.Windows.Forms.TrackBar timingBias;
        private System.Windows.Forms.Label forceRateLabel;
        private System.Windows.Forms.TextBox filterLengthValue;
        private System.Windows.Forms.TextBox timingBiasValue;
        private System.Windows.Forms.CheckBox forceHalf;
        private System.Windows.Forms.CheckBox forceThird;
        private System.Windows.Forms.Button restoreDefaults;
    }
}