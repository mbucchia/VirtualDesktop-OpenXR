
namespace companion
{
    partial class MainForm
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
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
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.openLogs = new System.Windows.Forms.Button();
            this.startTrace = new System.Windows.Forms.Button();
            this.stopTrace = new System.Windows.Forms.Button();
            this.flowLayoutPanel3 = new System.Windows.Forms.FlowLayoutPanel();
            this.label1 = new System.Windows.Forms.Label();
            this.runtimePimax = new System.Windows.Forms.RadioButton();
            this.runtimeSteam = new System.Windows.Forms.RadioButton();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.gotoDownloads = new System.Windows.Forms.LinkLabel();
            this.reportIssues = new System.Windows.Forms.LinkLabel();
            this.flowLayoutPanel4 = new System.Windows.Forms.FlowLayoutPanel();
            this.label2 = new System.Windows.Forms.Label();
            this.versionString = new System.Windows.Forms.Label();
            this.recenterMode = new System.Windows.Forms.CheckBox();
            this.label3 = new System.Windows.Forms.Label();
            this.tableLayoutPanel1.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.flowLayoutPanel3.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.flowLayoutPanel4.SuspendLayout();
            this.SuspendLayout();
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel1, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel3, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel2, 0, 3);
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel4, 0, 1);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Margin = new System.Windows.Forms.Padding(2);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 4;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 18.60465F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 81.39535F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 56F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(406, 246);
            this.tableLayoutPanel1.TabIndex = 0;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.openLogs);
            this.flowLayoutPanel1.Controls.Add(this.startTrace);
            this.flowLayoutPanel1.Controls.Add(this.stopTrace);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(2, 171);
            this.flowLayoutPanel1.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(402, 52);
            this.flowLayoutPanel1.TabIndex = 7;
            // 
            // openLogs
            // 
            this.openLogs.Location = new System.Drawing.Point(2, 2);
            this.openLogs.Margin = new System.Windows.Forms.Padding(2);
            this.openLogs.Name = "openLogs";
            this.openLogs.Size = new System.Drawing.Size(130, 42);
            this.openLogs.TabIndex = 7;
            this.openLogs.Text = "Open logs";
            this.openLogs.UseVisualStyleBackColor = true;
            this.openLogs.Click += new System.EventHandler(this.openLogs_Click);
            // 
            // startTrace
            // 
            this.startTrace.Location = new System.Drawing.Point(136, 2);
            this.startTrace.Margin = new System.Windows.Forms.Padding(2);
            this.startTrace.Name = "startTrace";
            this.startTrace.Size = new System.Drawing.Size(130, 42);
            this.startTrace.TabIndex = 8;
            this.startTrace.Text = "Capture trace";
            this.startTrace.UseVisualStyleBackColor = true;
            this.startTrace.Click += new System.EventHandler(this.startTrace_Click);
            // 
            // stopTrace
            // 
            this.stopTrace.Enabled = false;
            this.stopTrace.Location = new System.Drawing.Point(270, 2);
            this.stopTrace.Margin = new System.Windows.Forms.Padding(2);
            this.stopTrace.Name = "stopTrace";
            this.stopTrace.Size = new System.Drawing.Size(130, 42);
            this.stopTrace.TabIndex = 9;
            this.stopTrace.Text = "Stop capture";
            this.stopTrace.UseVisualStyleBackColor = true;
            this.stopTrace.Click += new System.EventHandler(this.stopTrace_Click);
            // 
            // flowLayoutPanel3
            // 
            this.flowLayoutPanel3.Controls.Add(this.label1);
            this.flowLayoutPanel3.Controls.Add(this.runtimePimax);
            this.flowLayoutPanel3.Controls.Add(this.runtimeSteam);
            this.flowLayoutPanel3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel3.Location = new System.Drawing.Point(2, 2);
            this.flowLayoutPanel3.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel3.Name = "flowLayoutPanel3";
            this.flowLayoutPanel3.Size = new System.Drawing.Size(402, 27);
            this.flowLayoutPanel3.TabIndex = 0;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Dock = System.Windows.Forms.DockStyle.Top;
            this.label1.Location = new System.Drawing.Point(2, 0);
            this.label1.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label1.Name = "label1";
            this.label1.Padding = new System.Windows.Forms.Padding(0, 3, 0, 0);
            this.label1.Size = new System.Drawing.Size(153, 16);
            this.label1.TabIndex = 0;
            this.label1.Text = "Select active OpenXR runtime:";
            // 
            // runtimePimax
            // 
            this.runtimePimax.AutoSize = true;
            this.runtimePimax.Location = new System.Drawing.Point(159, 2);
            this.runtimePimax.Margin = new System.Windows.Forms.Padding(2);
            this.runtimePimax.Name = "runtimePimax";
            this.runtimePimax.Size = new System.Drawing.Size(68, 17);
            this.runtimePimax.TabIndex = 1;
            this.runtimePimax.TabStop = true;
            this.runtimePimax.Text = "PimaxXR";
            this.runtimePimax.UseVisualStyleBackColor = true;
            this.runtimePimax.CheckedChanged += new System.EventHandler(this.runtimePimax_CheckedChanged);
            // 
            // runtimeSteam
            // 
            this.runtimeSteam.AutoSize = true;
            this.runtimeSteam.Location = new System.Drawing.Point(231, 2);
            this.runtimeSteam.Margin = new System.Windows.Forms.Padding(2);
            this.runtimeSteam.Name = "runtimeSteam";
            this.runtimeSteam.Size = new System.Drawing.Size(70, 17);
            this.runtimeSteam.TabIndex = 2;
            this.runtimeSteam.TabStop = true;
            this.runtimeSteam.Text = "SteamVR";
            this.runtimeSteam.UseVisualStyleBackColor = true;
            this.runtimeSteam.CheckedChanged += new System.EventHandler(this.runtimeSteam_CheckedChanged);
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.Controls.Add(this.gotoDownloads);
            this.flowLayoutPanel2.Controls.Add(this.reportIssues);
            this.flowLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(2, 227);
            this.flowLayoutPanel2.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(402, 17);
            this.flowLayoutPanel2.TabIndex = 10;
            // 
            // gotoDownloads
            // 
            this.gotoDownloads.AutoSize = true;
            this.gotoDownloads.Location = new System.Drawing.Point(2, 0);
            this.gotoDownloads.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.gotoDownloads.Name = "gotoDownloads";
            this.gotoDownloads.Size = new System.Drawing.Size(87, 13);
            this.gotoDownloads.TabIndex = 10;
            this.gotoDownloads.TabStop = true;
            this.gotoDownloads.Text = "Go to downloads";
            this.gotoDownloads.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.gotoDownloads_LinkClicked);
            // 
            // reportIssues
            // 
            this.reportIssues.AutoSize = true;
            this.reportIssues.Location = new System.Drawing.Point(93, 0);
            this.reportIssues.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.reportIssues.Name = "reportIssues";
            this.reportIssues.Size = new System.Drawing.Size(71, 13);
            this.reportIssues.TabIndex = 11;
            this.reportIssues.TabStop = true;
            this.reportIssues.Text = "Report issues";
            this.reportIssues.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.reportIssues_LinkClicked);
            // 
            // flowLayoutPanel4
            // 
            this.flowLayoutPanel4.Controls.Add(this.label2);
            this.flowLayoutPanel4.Controls.Add(this.versionString);
            this.flowLayoutPanel4.Controls.Add(this.label3);
            this.flowLayoutPanel4.Controls.Add(this.recenterMode);
            this.flowLayoutPanel4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel4.Location = new System.Drawing.Point(2, 33);
            this.flowLayoutPanel4.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel4.Name = "flowLayoutPanel4";
            this.flowLayoutPanel4.Size = new System.Drawing.Size(402, 134);
            this.flowLayoutPanel4.TabIndex = 3;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(2, 0);
            this.label2.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(90, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "PimaxXR version:";
            // 
            // versionString
            // 
            this.versionString.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.versionString, true);
            this.versionString.Location = new System.Drawing.Point(96, 0);
            this.versionString.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.versionString.Name = "versionString";
            this.versionString.Size = new System.Drawing.Size(53, 13);
            this.versionString.TabIndex = 4;
            this.versionString.Text = "Unknown";
            // 
            // recenterMode
            // 
            this.recenterMode.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.recenterMode, true);
            this.recenterMode.Location = new System.Drawing.Point(2, 51);
            this.recenterMode.Margin = new System.Windows.Forms.Padding(2);
            this.recenterMode.Name = "recenterMode";
            this.recenterMode.Padding = new System.Windows.Forms.Padding(3, 3, 0, 0);
            this.recenterMode.Size = new System.Drawing.Size(164, 20);
            this.recenterMode.TabIndex = 6;
            this.recenterMode.Text = "Recenter headset on startup";
            this.recenterMode.UseVisualStyleBackColor = true;
            this.recenterMode.CheckedChanged += new System.EventHandler(this.recenterMode_CheckedChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.label3, true);
            this.label3.Location = new System.Drawing.Point(3, 13);
            this.label3.Name = "label3";
            this.label3.Padding = new System.Windows.Forms.Padding(3, 10, 0, 0);
            this.label3.Size = new System.Drawing.Size(389, 36);
            this.label3.TabIndex = 5;
            this.label3.Text = "Use PiTool to set refresh rate, resolution, FOV, enable Smart Smoothing, Parallel" +
    " Projection, etc...";
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(406, 246);
            this.Controls.Add(this.tableLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Margin = new System.Windows.Forms.Padding(2, 1, 2, 1);
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.Text = "PimaxXR - OpenXR Control Center";
            this.tableLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel3.ResumeLayout(false);
            this.flowLayoutPanel3.PerformLayout();
            this.flowLayoutPanel2.ResumeLayout(false);
            this.flowLayoutPanel2.PerformLayout();
            this.flowLayoutPanel4.ResumeLayout(false);
            this.flowLayoutPanel4.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.Button openLogs;
        private System.Windows.Forms.Button startTrace;
        private System.Windows.Forms.Button stopTrace;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel3;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.RadioButton runtimePimax;
        private System.Windows.Forms.RadioButton runtimeSteam;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private System.Windows.Forms.LinkLabel gotoDownloads;
        private System.Windows.Forms.LinkLabel reportIssues;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel4;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label versionString;
        private System.Windows.Forms.CheckBox recenterMode;
        private System.Windows.Forms.Label label3;
    }
}

