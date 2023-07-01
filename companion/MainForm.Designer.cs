
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
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
            this.pitoolLabel = new System.Windows.Forms.Label();
            this.runtimeStatusLabel = new System.Windows.Forms.Label();
            this.recenterMode = new System.Windows.Forms.CheckBox();
            this.recenterLabel = new System.Windows.Forms.Label();
            this.controllerEmulation = new System.Windows.Forms.ComboBox();
            this.controllerEmulationLabel = new System.Windows.Forms.Label();
            this.joystickDeadzoneValue = new System.Windows.Forms.TextBox();
            this.joystickLabel = new System.Windows.Forms.Label();
            this.joystickDeadzone = new System.Windows.Forms.TrackBar();
            this.guardian = new System.Windows.Forms.CheckBox();
            this.guardianRadiusValue = new System.Windows.Forms.TextBox();
            this.guardianLabel1 = new System.Windows.Forms.Label();
            this.guardianRadius = new System.Windows.Forms.TrackBar();
            this.guardianThresholdValue = new System.Windows.Forms.TextBox();
            this.guardianLabel2 = new System.Windows.Forms.Label();
            this.guardianThreshold = new System.Windows.Forms.TrackBar();
            this.preferFramerate = new System.Windows.Forms.CheckBox();
            this.allowEyeTracking = new System.Windows.Forms.CheckBox();
            this.enableQuadViews = new System.Windows.Forms.CheckBox();
            this.enableUltraleap = new System.Windows.Forms.CheckBox();
            this.downloadUltraleap = new System.Windows.Forms.LinkLabel();
            this.mirrorMode = new System.Windows.Forms.CheckBox();
            this.telemetryLabel = new System.Windows.Forms.Label();
            this.enableTelemetry = new System.Windows.Forms.CheckBox();
            this.restoreDefaults = new System.Windows.Forms.Button();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.toolTip = new System.Windows.Forms.ToolTip(this.components);
            this.tableLayoutPanel1.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.flowLayoutPanel3.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.flowLayoutPanel4.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.joystickDeadzone)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.guardianRadius)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.guardianThreshold)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel1, 0, 3);
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel3, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel2, 0, 4);
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel4, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.pictureBox1, 0, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Margin = new System.Windows.Forms.Padding(2);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 5;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 121F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 26F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 56F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(406, 841);
            this.tableLayoutPanel1.TabIndex = 0;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.openLogs);
            this.flowLayoutPanel1.Controls.Add(this.startTrace);
            this.flowLayoutPanel1.Controls.Add(this.stopTrace);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(2, 767);
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
            this.openLogs.TabIndex = 43;
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
            this.startTrace.TabIndex = 44;
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
            this.stopTrace.TabIndex = 45;
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
            this.flowLayoutPanel3.Location = new System.Drawing.Point(2, 123);
            this.flowLayoutPanel3.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel3.Name = "flowLayoutPanel3";
            this.flowLayoutPanel3.Size = new System.Drawing.Size(402, 22);
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
            this.flowLayoutPanel2.Location = new System.Drawing.Point(2, 823);
            this.flowLayoutPanel2.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(402, 16);
            this.flowLayoutPanel2.TabIndex = 100;
            // 
            // gotoDownloads
            // 
            this.gotoDownloads.AutoSize = true;
            this.gotoDownloads.Location = new System.Drawing.Point(2, 0);
            this.gotoDownloads.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.gotoDownloads.Name = "gotoDownloads";
            this.gotoDownloads.Size = new System.Drawing.Size(87, 13);
            this.gotoDownloads.TabIndex = 100;
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
            this.reportIssues.TabIndex = 101;
            this.reportIssues.TabStop = true;
            this.reportIssues.Text = "Report issues";
            this.reportIssues.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.reportIssues_LinkClicked);
            // 
            // flowLayoutPanel4
            // 
            this.flowLayoutPanel4.Controls.Add(this.label2);
            this.flowLayoutPanel4.Controls.Add(this.versionString);
            this.flowLayoutPanel4.Controls.Add(this.pitoolLabel);
            this.flowLayoutPanel4.Controls.Add(this.runtimeStatusLabel);
            this.flowLayoutPanel4.Controls.Add(this.recenterMode);
            this.flowLayoutPanel4.Controls.Add(this.recenterLabel);
            this.flowLayoutPanel4.Controls.Add(this.controllerEmulation);
            this.flowLayoutPanel4.Controls.Add(this.controllerEmulationLabel);
            this.flowLayoutPanel4.Controls.Add(this.joystickDeadzoneValue);
            this.flowLayoutPanel4.Controls.Add(this.joystickLabel);
            this.flowLayoutPanel4.Controls.Add(this.joystickDeadzone);
            this.flowLayoutPanel4.Controls.Add(this.guardian);
            this.flowLayoutPanel4.Controls.Add(this.guardianRadiusValue);
            this.flowLayoutPanel4.Controls.Add(this.guardianLabel1);
            this.flowLayoutPanel4.Controls.Add(this.guardianRadius);
            this.flowLayoutPanel4.Controls.Add(this.guardianThresholdValue);
            this.flowLayoutPanel4.Controls.Add(this.guardianLabel2);
            this.flowLayoutPanel4.Controls.Add(this.guardianThreshold);
            this.flowLayoutPanel4.Controls.Add(this.preferFramerate);
            this.flowLayoutPanel4.Controls.Add(this.allowEyeTracking);
            this.flowLayoutPanel4.Controls.Add(this.enableQuadViews);
            this.flowLayoutPanel4.Controls.Add(this.enableUltraleap);
            this.flowLayoutPanel4.Controls.Add(this.downloadUltraleap);
            this.flowLayoutPanel4.Controls.Add(this.mirrorMode);
            this.flowLayoutPanel4.Controls.Add(this.telemetryLabel);
            this.flowLayoutPanel4.Controls.Add(this.enableTelemetry);
            this.flowLayoutPanel4.Controls.Add(this.restoreDefaults);
            this.flowLayoutPanel4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel4.Location = new System.Drawing.Point(2, 149);
            this.flowLayoutPanel4.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel4.Name = "flowLayoutPanel4";
            this.flowLayoutPanel4.Size = new System.Drawing.Size(402, 614);
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
            // pitoolLabel
            // 
            this.pitoolLabel.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.pitoolLabel, true);
            this.pitoolLabel.Location = new System.Drawing.Point(3, 13);
            this.pitoolLabel.Name = "pitoolLabel";
            this.pitoolLabel.Padding = new System.Windows.Forms.Padding(3, 10, 0, 0);
            this.pitoolLabel.Size = new System.Drawing.Size(368, 36);
            this.pitoolLabel.TabIndex = 5;
            this.pitoolLabel.Text = "Use PiTool or Pimax Client to set refresh rate, resolution, FOV, enable Smart Smo" +
    "othing, Parallel Projection, etc...";
            // 
            // runtimeStatusLabel
            // 
            this.flowLayoutPanel4.SetFlowBreak(this.runtimeStatusLabel, true);
            this.runtimeStatusLabel.Location = new System.Drawing.Point(3, 58);
            this.runtimeStatusLabel.Margin = new System.Windows.Forms.Padding(3, 9, 3, 6);
            this.runtimeStatusLabel.Name = "runtimeStatusLabel";
            this.runtimeStatusLabel.Size = new System.Drawing.Size(400, 26);
            this.runtimeStatusLabel.TabIndex = 6;
            this.runtimeStatusLabel.Click += new System.EventHandler(this.runtimeStatusLabel_Click);
            // 
            // recenterMode
            // 
            this.recenterMode.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.recenterMode, true);
            this.recenterMode.Location = new System.Drawing.Point(2, 92);
            this.recenterMode.Margin = new System.Windows.Forms.Padding(2);
            this.recenterMode.Name = "recenterMode";
            this.recenterMode.Padding = new System.Windows.Forms.Padding(3, 9, 0, 0);
            this.recenterMode.Size = new System.Drawing.Size(164, 26);
            this.recenterMode.TabIndex = 10;
            this.recenterMode.Text = "Recenter headset on startup";
            this.recenterMode.UseVisualStyleBackColor = true;
            this.recenterMode.CheckedChanged += new System.EventHandler(this.recenterMode_CheckedChanged);
            // 
            // recenterLabel
            // 
            this.recenterLabel.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.recenterLabel, true);
            this.recenterLabel.Location = new System.Drawing.Point(3, 120);
            this.recenterLabel.Name = "recenterLabel";
            this.recenterLabel.Padding = new System.Windows.Forms.Padding(3, 0, 0, 21);
            this.recenterLabel.Size = new System.Drawing.Size(385, 47);
            this.recenterLabel.TabIndex = 11;
            this.recenterLabel.Text = "You may also recenter in-game by holding for 2 seconds the System button and the " +
    "Trigger on any motion controller, or Ctrl+Alt+Space on your keyboard.";
            // 
            // controllerEmulation
            // 
            this.controllerEmulation.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.controllerEmulation.FormattingEnabled = true;
            this.controllerEmulation.ItemHeight = 13;
            this.controllerEmulation.Items.AddRange(new object[] {
            "",
            "Oculus Touch",
            "Windows Mixed Reality"});
            this.controllerEmulation.Location = new System.Drawing.Point(3, 170);
            this.controllerEmulation.Name = "controllerEmulation";
            this.controllerEmulation.Size = new System.Drawing.Size(128, 21);
            this.controllerEmulation.TabIndex = 12;
            this.controllerEmulation.SelectedIndexChanged += new System.EventHandler(this.controllerEmulation_SelectedIndexChanged);
            // 
            // controllerEmulationLabel
            // 
            this.controllerEmulationLabel.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.controllerEmulationLabel, true);
            this.controllerEmulationLabel.Location = new System.Drawing.Point(137, 167);
            this.controllerEmulationLabel.Name = "controllerEmulationLabel";
            this.controllerEmulationLabel.Padding = new System.Windows.Forms.Padding(0, 6, 0, 0);
            this.controllerEmulationLabel.Size = new System.Drawing.Size(226, 19);
            this.controllerEmulationLabel.TabIndex = 13;
            this.controllerEmulationLabel.Text = "Controller emulation (may fix incorrect bindings)";
            // 
            // joystickDeadzoneValue
            // 
            this.joystickDeadzoneValue.AccessibleDescription = "Value for joystick deadzone";
            this.joystickDeadzoneValue.Location = new System.Drawing.Point(3, 197);
            this.joystickDeadzoneValue.Name = "joystickDeadzoneValue";
            this.joystickDeadzoneValue.ReadOnly = true;
            this.joystickDeadzoneValue.Size = new System.Drawing.Size(26, 20);
            this.joystickDeadzoneValue.TabIndex = 14;
            // 
            // joystickLabel
            // 
            this.joystickLabel.AutoSize = true;
            this.joystickLabel.Location = new System.Drawing.Point(35, 194);
            this.joystickLabel.Name = "joystickLabel";
            this.joystickLabel.Padding = new System.Windows.Forms.Padding(0, 9, 0, 0);
            this.joystickLabel.Size = new System.Drawing.Size(98, 22);
            this.joystickLabel.TabIndex = 15;
            this.joystickLabel.Text = "Joystick deadzone:";
            // 
            // joystickDeadzone
            // 
            this.joystickDeadzone.AccessibleDescription = "Slider for joystick deadzone";
            this.joystickDeadzone.Location = new System.Drawing.Point(139, 197);
            this.joystickDeadzone.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.joystickDeadzone.Maximum = 100;
            this.joystickDeadzone.Name = "joystickDeadzone";
            this.joystickDeadzone.Size = new System.Drawing.Size(104, 45);
            this.joystickDeadzone.TabIndex = 16;
            this.joystickDeadzone.Scroll += new System.EventHandler(this.joystickDeadzone_Scroll);
            // 
            // guardian
            // 
            this.guardian.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.guardian, true);
            this.guardian.Location = new System.Drawing.Point(2, 244);
            this.guardian.Margin = new System.Windows.Forms.Padding(2);
            this.guardian.Name = "guardian";
            this.guardian.Padding = new System.Windows.Forms.Padding(3, 3, 0, 0);
            this.guardian.Size = new System.Drawing.Size(187, 20);
            this.guardian.TabIndex = 20;
            this.guardian.Text = "Enable visual playspace guardian";
            this.guardian.UseVisualStyleBackColor = true;
            this.guardian.CheckedChanged += new System.EventHandler(this.guardian_CheckedChanged);
            // 
            // guardianRadiusValue
            // 
            this.guardianRadiusValue.AccessibleDescription = "Value for guardian radius";
            this.guardianRadiusValue.Location = new System.Drawing.Point(30, 271);
            this.guardianRadiusValue.Margin = new System.Windows.Forms.Padding(30, 3, 3, 3);
            this.guardianRadiusValue.Name = "guardianRadiusValue";
            this.guardianRadiusValue.ReadOnly = true;
            this.guardianRadiusValue.Size = new System.Drawing.Size(26, 20);
            this.guardianRadiusValue.TabIndex = 21;
            // 
            // guardianLabel1
            // 
            this.guardianLabel1.AutoSize = true;
            this.guardianLabel1.Location = new System.Drawing.Point(59, 268);
            this.guardianLabel1.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
            this.guardianLabel1.Name = "guardianLabel1";
            this.guardianLabel1.Padding = new System.Windows.Forms.Padding(0, 9, 0, 0);
            this.guardianLabel1.Size = new System.Drawing.Size(49, 22);
            this.guardianLabel1.TabIndex = 22;
            this.guardianLabel1.Text = "m radius:";
            // 
            // guardianRadius
            // 
            this.guardianRadius.AccessibleDescription = "Slider for guardian radius";
            this.flowLayoutPanel4.SetFlowBreak(this.guardianRadius, true);
            this.guardianRadius.Location = new System.Drawing.Point(114, 271);
            this.guardianRadius.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.guardianRadius.Maximum = 600;
            this.guardianRadius.Minimum = 50;
            this.guardianRadius.Name = "guardianRadius";
            this.guardianRadius.Size = new System.Drawing.Size(104, 45);
            this.guardianRadius.TabIndex = 23;
            this.guardianRadius.TickFrequency = 10;
            this.guardianRadius.Value = 100;
            this.guardianRadius.Scroll += new System.EventHandler(this.guardianRadius_Scroll);
            // 
            // guardianThresholdValue
            // 
            this.guardianThresholdValue.AccessibleDescription = "Value for guardian visibility";
            this.guardianThresholdValue.Location = new System.Drawing.Point(30, 319);
            this.guardianThresholdValue.Margin = new System.Windows.Forms.Padding(30, 3, 3, 3);
            this.guardianThresholdValue.Name = "guardianThresholdValue";
            this.guardianThresholdValue.ReadOnly = true;
            this.guardianThresholdValue.Size = new System.Drawing.Size(26, 20);
            this.guardianThresholdValue.TabIndex = 24;
            // 
            // guardianLabel2
            // 
            this.guardianLabel2.AutoSize = true;
            this.guardianLabel2.Location = new System.Drawing.Point(59, 316);
            this.guardianLabel2.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
            this.guardianLabel2.Name = "guardianLabel2";
            this.guardianLabel2.Padding = new System.Windows.Forms.Padding(0, 9, 0, 0);
            this.guardianLabel2.Size = new System.Drawing.Size(56, 22);
            this.guardianLabel2.TabIndex = 25;
            this.guardianLabel2.Text = "m visibility:";
            // 
            // guardianThreshold
            // 
            this.guardianThreshold.AccessibleDescription = "Slider for guardian visibility";
            this.flowLayoutPanel4.SetFlowBreak(this.guardianThreshold, true);
            this.guardianThreshold.Location = new System.Drawing.Point(121, 319);
            this.guardianThreshold.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.guardianThreshold.Maximum = 600;
            this.guardianThreshold.Minimum = 50;
            this.guardianThreshold.Name = "guardianThreshold";
            this.guardianThreshold.Size = new System.Drawing.Size(104, 45);
            this.guardianThreshold.TabIndex = 26;
            this.guardianThreshold.TickFrequency = 10;
            this.guardianThreshold.Value = 100;
            this.guardianThreshold.Scroll += new System.EventHandler(this.guardianThreshold_Scroll);
            // 
            // preferFramerate
            // 
            this.preferFramerate.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.preferFramerate, true);
            this.preferFramerate.Location = new System.Drawing.Point(3, 367);
            this.preferFramerate.Name = "preferFramerate";
            this.preferFramerate.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.preferFramerate.Size = new System.Drawing.Size(338, 17);
            this.preferFramerate.TabIndex = 30;
            this.preferFramerate.Text = "Prefer framerate over latency (incompatible with Smart Smoothing)";
            this.preferFramerate.UseVisualStyleBackColor = true;
            this.preferFramerate.CheckedChanged += new System.EventHandler(this.preferFramerate_CheckedChanged);
            // 
            // allowEyeTracking
            // 
            this.allowEyeTracking.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.allowEyeTracking, true);
            this.allowEyeTracking.Location = new System.Drawing.Point(3, 390);
            this.allowEyeTracking.Name = "allowEyeTracking";
            this.allowEyeTracking.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.allowEyeTracking.Size = new System.Drawing.Size(301, 17);
            this.allowEyeTracking.TabIndex = 31;
            this.allowEyeTracking.Text = "Allow use of the eye tracker (Pimax Crystal or Droolon Pi1)";
            this.allowEyeTracking.UseVisualStyleBackColor = true;
            this.allowEyeTracking.CheckedChanged += new System.EventHandler(this.allowEyeTracking_CheckedChanged);
            // 
            // enableQuadViews
            // 
            this.enableQuadViews.AutoSize = true;
            this.enableQuadViews.Enabled = false;
            this.flowLayoutPanel4.SetFlowBreak(this.enableQuadViews, true);
            this.enableQuadViews.Location = new System.Drawing.Point(3, 413);
            this.enableQuadViews.Name = "enableQuadViews";
            this.enableQuadViews.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.enableQuadViews.Size = new System.Drawing.Size(306, 17);
            this.enableQuadViews.TabIndex = 32;
            this.enableQuadViews.Text = "Enable Quad Views rendering (with supported applications)";
            this.enableQuadViews.UseVisualStyleBackColor = true;
            this.enableQuadViews.CheckedChanged += new System.EventHandler(this.enableQuadViews_CheckedChanged);
            // 
            // enableUltraleap
            // 
            this.enableUltraleap.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.enableUltraleap, true);
            this.enableUltraleap.Location = new System.Drawing.Point(3, 436);
            this.enableUltraleap.Name = "enableUltraleap";
            this.enableUltraleap.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
            this.enableUltraleap.Size = new System.Drawing.Size(310, 17);
            this.enableUltraleap.TabIndex = 33;
            this.enableUltraleap.Text = "Enable Hand Tracking module (with supported applications)";
            this.enableUltraleap.UseVisualStyleBackColor = true;
            this.enableUltraleap.CheckedChanged += new System.EventHandler(this.enableUltraleap_CheckedChanged);
            // 
            // downloadUltraleap
            // 
            this.downloadUltraleap.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.downloadUltraleap, true);
            this.downloadUltraleap.Location = new System.Drawing.Point(3, 456);
            this.downloadUltraleap.Name = "downloadUltraleap";
            this.downloadUltraleap.Padding = new System.Windows.Forms.Padding(18, 0, 0, 0);
            this.downloadUltraleap.Size = new System.Drawing.Size(283, 13);
            this.downloadUltraleap.TabIndex = 34;
            this.downloadUltraleap.TabStop = true;
            this.downloadUltraleap.Text = "Download or update Ultraleap Hand Tracking software";
            this.downloadUltraleap.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.downloadUltraleap_LinkClicked);
            // 
            // mirrorMode
            // 
            this.mirrorMode.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.mirrorMode, true);
            this.mirrorMode.Location = new System.Drawing.Point(3, 472);
            this.mirrorMode.Name = "mirrorMode";
            this.mirrorMode.Padding = new System.Windows.Forms.Padding(3, 3, 0, 0);
            this.mirrorMode.Size = new System.Drawing.Size(123, 20);
            this.mirrorMode.TabIndex = 35;
            this.mirrorMode.Text = "Show mirror window";
            this.mirrorMode.UseVisualStyleBackColor = true;
            this.mirrorMode.CheckedChanged += new System.EventHandler(this.mirrorMode_CheckedChanged);
            // 
            // telemetryLabel
            // 
            this.telemetryLabel.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.telemetryLabel, true);
            this.telemetryLabel.Location = new System.Drawing.Point(3, 495);
            this.telemetryLabel.Name = "telemetryLabel";
            this.telemetryLabel.Padding = new System.Windows.Forms.Padding(3, 9, 0, 0);
            this.telemetryLabel.Size = new System.Drawing.Size(395, 35);
            this.telemetryLabel.TabIndex = 40;
            this.telemetryLabel.Text = "Our telemetry does not affect performance, is anonymous and helps the developer w" +
    "ith application support.";
            // 
            // enableTelemetry
            // 
            this.enableTelemetry.AutoSize = true;
            this.flowLayoutPanel4.SetFlowBreak(this.enableTelemetry, true);
            this.enableTelemetry.Location = new System.Drawing.Point(2, 532);
            this.enableTelemetry.Margin = new System.Windows.Forms.Padding(2);
            this.enableTelemetry.Name = "enableTelemetry";
            this.enableTelemetry.Padding = new System.Windows.Forms.Padding(3, 6, 0, 0);
            this.enableTelemetry.Size = new System.Drawing.Size(139, 23);
            this.enableTelemetry.TabIndex = 41;
            this.enableTelemetry.Text = "Enable usage telemetry";
            this.enableTelemetry.UseVisualStyleBackColor = true;
            this.enableTelemetry.CheckedChanged += new System.EventHandler(this.enableTelemetry_CheckedChanged);
            // 
            // restoreDefaults
            // 
            this.restoreDefaults.Location = new System.Drawing.Point(6, 569);
            this.restoreDefaults.Margin = new System.Windows.Forms.Padding(6, 0, 3, 0);
            this.restoreDefaults.Name = "restoreDefaults";
            this.restoreDefaults.Size = new System.Drawing.Size(126, 39);
            this.restoreDefaults.TabIndex = 42;
            this.restoreDefaults.Text = "Restore defaults";
            this.restoreDefaults.UseVisualStyleBackColor = true;
            this.restoreDefaults.Click += new System.EventHandler(this.restoreDefaults_Click);
            // 
            // pictureBox1
            // 
            this.pictureBox1.BackColor = System.Drawing.Color.White;
            this.pictureBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(3, 3);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(400, 115);
            this.pictureBox1.TabIndex = 0;
            this.pictureBox1.TabStop = false;
            this.pictureBox1.DoubleClick += new System.EventHandler(this.pictureBox1_DoubleClick);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(406, 841);
            this.Controls.Add(this.tableLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
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
            ((System.ComponentModel.ISupportInitialize)(this.joystickDeadzone)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.guardianRadius)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.guardianThreshold)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
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
        private System.Windows.Forms.Label versionString;
        private System.Windows.Forms.CheckBox recenterMode;
        private System.Windows.Forms.Label pitoolLabel;
        private System.Windows.Forms.Label telemetryLabel;
        private System.Windows.Forms.CheckBox enableTelemetry;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label joystickLabel;
        private System.Windows.Forms.TrackBar joystickDeadzone;
        private System.Windows.Forms.TextBox joystickDeadzoneValue;
        private System.Windows.Forms.CheckBox guardian;
        private System.Windows.Forms.Label guardianLabel1;
        private System.Windows.Forms.TrackBar guardianRadius;
        private System.Windows.Forms.TextBox guardianRadiusValue;
        private System.Windows.Forms.Label guardianLabel2;
        private System.Windows.Forms.TrackBar guardianThreshold;
        private System.Windows.Forms.TextBox guardianThresholdValue;
        private System.Windows.Forms.ComboBox controllerEmulation;
        private System.Windows.Forms.Label controllerEmulationLabel;
        private System.Windows.Forms.Label runtimeStatusLabel;
        private System.Windows.Forms.Label recenterLabel;
        private System.Windows.Forms.CheckBox mirrorMode;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button restoreDefaults;
        private System.Windows.Forms.CheckBox allowEyeTracking;
        private System.Windows.Forms.CheckBox enableQuadViews;
        private System.Windows.Forms.CheckBox enableUltraleap;
        private System.Windows.Forms.LinkLabel downloadUltraleap;
        private System.Windows.Forms.ToolTip toolTip;
        private System.Windows.Forms.CheckBox preferFramerate;
    }
}

