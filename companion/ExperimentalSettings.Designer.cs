
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
            this.filterLengthValue = new System.Windows.Forms.TextBox();
            this.filterLengthLabel = new System.Windows.Forms.Label();
            this.filterLength = new System.Windows.Forms.TrackBar();
            this.timingBiasValue = new System.Windows.Forms.TextBox();
            this.timingBiasLabel = new System.Windows.Forms.Label();
            this.timingBias = new System.Windows.Forms.TrackBar();
            this.forceRateLabel = new System.Windows.Forms.Label();
            this.forceHalf = new System.Windows.Forms.CheckBox();
            this.forceThird = new System.Windows.Forms.CheckBox();
            this.disableFramePipelining = new System.Windows.Forms.CheckBox();
            this.alwaysUseFrameIdZero = new System.Windows.Forms.CheckBox();
            this.forceDisableParallelProjection = new System.Windows.Forms.CheckBox();
            this.droolonProjectionDistanceValue = new System.Windows.Forms.TextBox();
            this.droolonProjectionDistanceLabel = new System.Windows.Forms.Label();
            this.droolonProjectionDistance = new System.Windows.Forms.TrackBar();
            this.restoreDefaults = new System.Windows.Forms.Button();
            this.flowLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.filterLength)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.timingBias)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.droolonProjectionDistance)).BeginInit();
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
            this.flowLayoutPanel1.Controls.Add(this.disableFramePipelining);
            this.flowLayoutPanel1.Controls.Add(this.alwaysUseFrameIdZero);
            this.flowLayoutPanel1.Controls.Add(this.forceDisableParallelProjection);
            this.flowLayoutPanel1.Controls.Add(this.droolonProjectionDistanceValue);
            this.flowLayoutPanel1.Controls.Add(this.droolonProjectionDistanceLabel);
            this.flowLayoutPanel1.Controls.Add(this.droolonProjectionDistance);
            this.flowLayoutPanel1.Controls.Add(this.restoreDefaults);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(300, 344);
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
            // filterLengthValue
            // 
            this.filterLengthValue.Location = new System.Drawing.Point(6, 35);
            this.filterLengthValue.Margin = new System.Windows.Forms.Padding(6, 6, 3, 3);
            this.filterLengthValue.Name = "filterLengthValue";
            this.filterLengthValue.ReadOnly = true;
            this.filterLengthValue.Size = new System.Drawing.Size(32, 20);
            this.filterLengthValue.TabIndex = 1;
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
            // timingBiasValue
            // 
            this.timingBiasValue.Location = new System.Drawing.Point(6, 80);
            this.timingBiasValue.Margin = new System.Windows.Forms.Padding(6, 3, 3, 3);
            this.timingBiasValue.Name = "timingBiasValue";
            this.timingBiasValue.ReadOnly = true;
            this.timingBiasValue.Size = new System.Drawing.Size(32, 20);
            this.timingBiasValue.TabIndex = 4;
            // 
            // timingBiasLabel
            // 
            this.timingBiasLabel.AutoSize = true;
            this.timingBiasLabel.Location = new System.Drawing.Point(44, 77);
            this.timingBiasLabel.Name = "timingBiasLabel";
            this.timingBiasLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
            this.timingBiasLabel.Size = new System.Drawing.Size(110, 19);
            this.timingBiasLabel.TabIndex = 5;
            this.timingBiasLabel.Text = "Frame Time Bias (ms):";
            // 
            // timingBias
            // 
            this.flowLayoutPanel1.SetFlowBreak(this.timingBias, true);
            this.timingBias.Location = new System.Drawing.Point(160, 80);
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
            // disableFramePipelining
            // 
            this.disableFramePipelining.AutoSize = true;
            this.flowLayoutPanel1.SetFlowBreak(this.disableFramePipelining, true);
            this.disableFramePipelining.Location = new System.Drawing.Point(3, 151);
            this.disableFramePipelining.Name = "disableFramePipelining";
            this.disableFramePipelining.Padding = new System.Windows.Forms.Padding(3, 9, 0, 0);
            this.disableFramePipelining.Size = new System.Drawing.Size(223, 26);
            this.disableFramePipelining.TabIndex = 10;
            this.disableFramePipelining.Text = "Disable frame pipelining (Requires restart)";
            this.disableFramePipelining.UseVisualStyleBackColor = true;
            this.disableFramePipelining.CheckedChanged += new System.EventHandler(this.disableFramePipelining_CheckedChanged);
            // 
            // alwaysUseFrameIdZero
            // 
            this.alwaysUseFrameIdZero.AutoSize = true;
            this.flowLayoutPanel1.SetFlowBreak(this.alwaysUseFrameIdZero, true);
            this.alwaysUseFrameIdZero.Location = new System.Drawing.Point(3, 183);
            this.alwaysUseFrameIdZero.Name = "alwaysUseFrameIdZero";
            this.alwaysUseFrameIdZero.Padding = new System.Windows.Forms.Padding(3, 6, 0, 0);
            this.alwaysUseFrameIdZero.Size = new System.Drawing.Size(227, 23);
            this.alwaysUseFrameIdZero.TabIndex = 11;
            this.alwaysUseFrameIdZero.Text = "Always use null frame ID (Requires restart)";
            this.alwaysUseFrameIdZero.UseVisualStyleBackColor = true;
            this.alwaysUseFrameIdZero.CheckedChanged += new System.EventHandler(this.alwaysUseFrameIdZero_CheckedChanged);
            // 
            // forceDisableParallelProjection
            // 
            this.forceDisableParallelProjection.AutoSize = true;
            this.forceDisableParallelProjection.Location = new System.Drawing.Point(3, 212);
            this.forceDisableParallelProjection.Name = "forceDisableParallelProjection";
            this.forceDisableParallelProjection.Padding = new System.Windows.Forms.Padding(3, 6, 0, 9);
            this.forceDisableParallelProjection.Size = new System.Drawing.Size(268, 32);
            this.forceDisableParallelProjection.TabIndex = 12;
            this.forceDisableParallelProjection.Text = "Force disabling parallel projection (Requires restart)";
            this.forceDisableParallelProjection.UseVisualStyleBackColor = true;
            this.forceDisableParallelProjection.CheckedChanged += new System.EventHandler(this.forceDisableParallelProjection_CheckedChanged);
            // 
            // droolonProjectionDistanceValue
            // 
            this.droolonProjectionDistanceValue.Location = new System.Drawing.Point(6, 253);
            this.droolonProjectionDistanceValue.Margin = new System.Windows.Forms.Padding(6, 6, 3, 3);
            this.droolonProjectionDistanceValue.Name = "droolonProjectionDistanceValue";
            this.droolonProjectionDistanceValue.ReadOnly = true;
            this.droolonProjectionDistanceValue.Size = new System.Drawing.Size(32, 20);
            this.droolonProjectionDistanceValue.TabIndex = 13;
            // 
            // droolonProjectionDistanceLabel
            // 
            this.droolonProjectionDistanceLabel.AutoSize = true;
            this.droolonProjectionDistanceLabel.Location = new System.Drawing.Point(44, 247);
            this.droolonProjectionDistanceLabel.Name = "droolonProjectionDistanceLabel";
            this.droolonProjectionDistanceLabel.Padding = new System.Windows.Forms.Padding(0, 9, 0, 0);
            this.droolonProjectionDistanceLabel.Size = new System.Drawing.Size(133, 22);
            this.droolonProjectionDistanceLabel.TabIndex = 14;
            this.droolonProjectionDistanceLabel.Text = "Droolon Proj. Distance: (m)";
            // 
            // droolonProjectionDistance
            // 
            this.flowLayoutPanel1.SetFlowBreak(this.droolonProjectionDistance, true);
            this.droolonProjectionDistance.LargeChange = 10;
            this.droolonProjectionDistance.Location = new System.Drawing.Point(183, 250);
            this.droolonProjectionDistance.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.droolonProjectionDistance.Maximum = 200;
            this.droolonProjectionDistance.Minimum = 10;
            this.droolonProjectionDistance.Name = "droolonProjectionDistance";
            this.droolonProjectionDistance.Size = new System.Drawing.Size(104, 45);
            this.droolonProjectionDistance.TabIndex = 15;
            this.droolonProjectionDistance.TickFrequency = 10;
            this.droolonProjectionDistance.Value = 10;
            this.droolonProjectionDistance.Scroll += new System.EventHandler(this.droolonProjectionDistance_Scroll);
            // 
            // restoreDefaults
            // 
            this.restoreDefaults.Location = new System.Drawing.Point(6, 298);
            this.restoreDefaults.Margin = new System.Windows.Forms.Padding(6, 3, 3, 3);
            this.restoreDefaults.Name = "restoreDefaults";
            this.restoreDefaults.Size = new System.Drawing.Size(126, 39);
            this.restoreDefaults.TabIndex = 16;
            this.restoreDefaults.Text = "Restore defaults";
            this.restoreDefaults.UseVisualStyleBackColor = true;
            this.restoreDefaults.Click += new System.EventHandler(this.restoreDefaults_Click);
            // 
            // ExperimentalSettings
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(300, 344);
            this.Controls.Add(this.flowLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ExperimentalSettings";
            this.Text = "PimaxXR - Experimental Settings";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.ExperimentalSettings_FormClosing);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.filterLength)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.timingBias)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.droolonProjectionDistance)).EndInit();
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
        private System.Windows.Forms.CheckBox disableFramePipelining;
        private System.Windows.Forms.CheckBox alwaysUseFrameIdZero;
        private System.Windows.Forms.CheckBox forceDisableParallelProjection;
        private System.Windows.Forms.Label droolonProjectionDistanceLabel;
        private System.Windows.Forms.TrackBar droolonProjectionDistance;
        private System.Windows.Forms.TextBox droolonProjectionDistanceValue;
    }
}