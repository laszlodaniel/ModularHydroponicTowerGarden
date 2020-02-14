namespace MHTG
{
    partial class MainForm
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
            this.MenuStrip = new System.Windows.Forms.MenuStrip();
            this.ToolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.UpdateFirmwareToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.ShowDebugToolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.ServiceModeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.AboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.CommunicationGroupBox = new System.Windows.Forms.GroupBox();
            this.SendButton = new System.Windows.Forms.Button();
            this.SendComboBox = new System.Windows.Forms.ComboBox();
            this.CommunicationTextBox = new System.Windows.Forms.TextBox();
            this.SettingsGroupBox = new System.Windows.Forms.GroupBox();
            this.NTCBetaValueModifyCheckBox = new System.Windows.Forms.CheckBox();
            this.NTCBetaKelvinLabel = new System.Windows.Forms.Label();
            this.NTCBetaValueLabel = new System.Windows.Forms.Label();
            this.NTCBetaValueTextBox = new System.Windows.Forms.TextBox();
            this.WaterPumpLabel = new System.Windows.Forms.Label();
            this.DefaultSettingsButton = new System.Windows.Forms.Button();
            this.WriteSettingsButton = new System.Windows.Forms.Button();
            this.ReadSettingsButton = new System.Windows.Forms.Button();
            this.TemperatureSensorsComboBox = new System.Windows.Forms.ComboBox();
            this.TemperatureSensorsLabel = new System.Windows.Forms.Label();
            this.TemperatureUnitComboBox = new System.Windows.Forms.ComboBox();
            this.TemperatureUnitLabel = new System.Windows.Forms.Label();
            this.MinutesLabel02 = new System.Windows.Forms.Label();
            this.WaterPumpOffTimeLabel = new System.Windows.Forms.Label();
            this.WaterPumpOffTimeTextBox = new System.Windows.Forms.TextBox();
            this.MinutesLabel01 = new System.Windows.Forms.Label();
            this.WaterPumpOnTimeTextBox = new System.Windows.Forms.TextBox();
            this.WaterPumpOnTimeLabel = new System.Windows.Forms.Label();
            this.WaterPumpSpeedPercentageLabel = new System.Windows.Forms.Label();
            this.WaterPumpSpeedLabel = new System.Windows.Forms.Label();
            this.WaterPumpSpeedTrackBar = new System.Windows.Forms.TrackBar();
            this.ExternalTemperatureTextBox = new System.Windows.Forms.TextBox();
            this.InternalTemperatureTextBox = new System.Windows.Forms.TextBox();
            this.ControlGroupBox = new System.Windows.Forms.GroupBox();
            this.StatusButton = new System.Windows.Forms.Button();
            this.ResetButton = new System.Windows.Forms.Button();
            this.RefreshButton = new System.Windows.Forms.Button();
            this.COMPortsComboBox = new System.Windows.Forms.ComboBox();
            this.ConnectButton = new System.Windows.Forms.Button();
            this.ExternalTemperatureLabel = new System.Windows.Forms.Label();
            this.InternalTemperatureLabel = new System.Windows.Forms.Label();
            this.TemperatureUnitLabel01 = new System.Windows.Forms.Label();
            this.TemperatureUnitLabel02 = new System.Windows.Forms.Label();
            this.TemperaturesGroupBox = new System.Windows.Forms.GroupBox();
            this.DebugGroupBox = new System.Windows.Forms.GroupBox();
            this.WaterPumpOnOffCheckBox = new System.Windows.Forms.CheckBox();
            this.GetStatusUpdatesCheckBox = new System.Windows.Forms.CheckBox();
            this.MenuStrip.SuspendLayout();
            this.CommunicationGroupBox.SuspendLayout();
            this.SettingsGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.WaterPumpSpeedTrackBar)).BeginInit();
            this.ControlGroupBox.SuspendLayout();
            this.TemperaturesGroupBox.SuspendLayout();
            this.DebugGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // MenuStrip
            // 
            this.MenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.ToolsToolStripMenuItem,
            this.AboutToolStripMenuItem});
            this.MenuStrip.Location = new System.Drawing.Point(0, 0);
            this.MenuStrip.Name = "MenuStrip";
            this.MenuStrip.Size = new System.Drawing.Size(764, 24);
            this.MenuStrip.TabIndex = 0;
            this.MenuStrip.Text = "menuStrip1";
            // 
            // ToolsToolStripMenuItem
            // 
            this.ToolsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.UpdateFirmwareToolStripMenuItem,
            this.ShowDebugToolsToolStripMenuItem,
            this.ServiceModeToolStripMenuItem});
            this.ToolsToolStripMenuItem.Name = "ToolsToolStripMenuItem";
            this.ToolsToolStripMenuItem.Size = new System.Drawing.Size(46, 20);
            this.ToolsToolStripMenuItem.Text = "Tools";
            // 
            // UpdateFirmwareToolStripMenuItem
            // 
            this.UpdateFirmwareToolStripMenuItem.Enabled = false;
            this.UpdateFirmwareToolStripMenuItem.Name = "UpdateFirmwareToolStripMenuItem";
            this.UpdateFirmwareToolStripMenuItem.Size = new System.Drawing.Size(180, 22);
            this.UpdateFirmwareToolStripMenuItem.Text = "Update firmware";
            this.UpdateFirmwareToolStripMenuItem.Click += new System.EventHandler(this.UpdateFirmwareToolStripMenuItem_Click);
            // 
            // ShowDebugToolsToolStripMenuItem
            // 
            this.ShowDebugToolsToolStripMenuItem.CheckOnClick = true;
            this.ShowDebugToolsToolStripMenuItem.Name = "ShowDebugToolsToolStripMenuItem";
            this.ShowDebugToolsToolStripMenuItem.Size = new System.Drawing.Size(169, 22);
            this.ShowDebugToolsToolStripMenuItem.Text = "Show debug tools";
            this.ShowDebugToolsToolStripMenuItem.CheckedChanged += new System.EventHandler(this.ShowDebugToolsToolStripMenuItem_CheckedChanged);
            // 
            // ServiceModeToolStripMenuItem
            // 
            this.ServiceModeToolStripMenuItem.CheckOnClick = true;
            this.ServiceModeToolStripMenuItem.Enabled = false;
            this.ServiceModeToolStripMenuItem.Name = "ServiceModeToolStripMenuItem";
            this.ServiceModeToolStripMenuItem.Size = new System.Drawing.Size(180, 22);
            this.ServiceModeToolStripMenuItem.Text = "Service mode";
            this.ServiceModeToolStripMenuItem.CheckedChanged += new System.EventHandler(this.ServiceModeToolStripMenuItem_CheckedChanged);
            // 
            // AboutToolStripMenuItem
            // 
            this.AboutToolStripMenuItem.Name = "AboutToolStripMenuItem";
            this.AboutToolStripMenuItem.Size = new System.Drawing.Size(52, 20);
            this.AboutToolStripMenuItem.Text = "About";
            this.AboutToolStripMenuItem.Click += new System.EventHandler(this.AboutToolStripMenuItem_Click);
            // 
            // CommunicationGroupBox
            // 
            this.CommunicationGroupBox.Controls.Add(this.SendButton);
            this.CommunicationGroupBox.Controls.Add(this.SendComboBox);
            this.CommunicationGroupBox.Controls.Add(this.CommunicationTextBox);
            this.CommunicationGroupBox.Location = new System.Drawing.Point(12, 27);
            this.CommunicationGroupBox.Name = "CommunicationGroupBox";
            this.CommunicationGroupBox.Size = new System.Drawing.Size(365, 265);
            this.CommunicationGroupBox.TabIndex = 1;
            this.CommunicationGroupBox.TabStop = false;
            this.CommunicationGroupBox.Text = "Communication";
            // 
            // SendButton
            // 
            this.SendButton.Location = new System.Drawing.Point(312, 237);
            this.SendButton.Name = "SendButton";
            this.SendButton.Size = new System.Drawing.Size(51, 25);
            this.SendButton.TabIndex = 2;
            this.SendButton.Text = "Send";
            this.SendButton.UseVisualStyleBackColor = true;
            this.SendButton.Click += new System.EventHandler(this.SendButton_Click);
            // 
            // SendComboBox
            // 
            this.SendComboBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.SendComboBox.FormattingEnabled = true;
            this.SendComboBox.Location = new System.Drawing.Point(3, 238);
            this.SendComboBox.Name = "SendComboBox";
            this.SendComboBox.Size = new System.Drawing.Size(308, 23);
            this.SendComboBox.TabIndex = 1;
            this.SendComboBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.SendComboBox_KeyPress);
            // 
            // CommunicationTextBox
            // 
            this.CommunicationTextBox.BackColor = System.Drawing.SystemColors.Window;
            this.CommunicationTextBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.CommunicationTextBox.Location = new System.Drawing.Point(3, 16);
            this.CommunicationTextBox.Multiline = true;
            this.CommunicationTextBox.Name = "CommunicationTextBox";
            this.CommunicationTextBox.ReadOnly = true;
            this.CommunicationTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.CommunicationTextBox.Size = new System.Drawing.Size(359, 220);
            this.CommunicationTextBox.TabIndex = 0;
            // 
            // SettingsGroupBox
            // 
            this.SettingsGroupBox.Controls.Add(this.NTCBetaValueModifyCheckBox);
            this.SettingsGroupBox.Controls.Add(this.NTCBetaKelvinLabel);
            this.SettingsGroupBox.Controls.Add(this.NTCBetaValueLabel);
            this.SettingsGroupBox.Controls.Add(this.NTCBetaValueTextBox);
            this.SettingsGroupBox.Controls.Add(this.WaterPumpLabel);
            this.SettingsGroupBox.Controls.Add(this.DefaultSettingsButton);
            this.SettingsGroupBox.Controls.Add(this.WriteSettingsButton);
            this.SettingsGroupBox.Controls.Add(this.ReadSettingsButton);
            this.SettingsGroupBox.Controls.Add(this.TemperatureSensorsComboBox);
            this.SettingsGroupBox.Controls.Add(this.TemperatureSensorsLabel);
            this.SettingsGroupBox.Controls.Add(this.TemperatureUnitComboBox);
            this.SettingsGroupBox.Controls.Add(this.TemperatureUnitLabel);
            this.SettingsGroupBox.Controls.Add(this.MinutesLabel02);
            this.SettingsGroupBox.Controls.Add(this.WaterPumpOffTimeLabel);
            this.SettingsGroupBox.Controls.Add(this.WaterPumpOffTimeTextBox);
            this.SettingsGroupBox.Controls.Add(this.MinutesLabel01);
            this.SettingsGroupBox.Controls.Add(this.WaterPumpOnTimeTextBox);
            this.SettingsGroupBox.Controls.Add(this.WaterPumpOnTimeLabel);
            this.SettingsGroupBox.Controls.Add(this.WaterPumpSpeedPercentageLabel);
            this.SettingsGroupBox.Controls.Add(this.WaterPumpSpeedLabel);
            this.SettingsGroupBox.Controls.Add(this.WaterPumpSpeedTrackBar);
            this.SettingsGroupBox.Location = new System.Drawing.Point(12, 296);
            this.SettingsGroupBox.Name = "SettingsGroupBox";
            this.SettingsGroupBox.Size = new System.Drawing.Size(365, 184);
            this.SettingsGroupBox.TabIndex = 2;
            this.SettingsGroupBox.TabStop = false;
            this.SettingsGroupBox.Text = "Settings";
            // 
            // NTCBetaValueModifyCheckBox
            // 
            this.NTCBetaValueModifyCheckBox.AutoSize = true;
            this.NTCBetaValueModifyCheckBox.Location = new System.Drawing.Point(304, 118);
            this.NTCBetaValueModifyCheckBox.Name = "NTCBetaValueModifyCheckBox";
            this.NTCBetaValueModifyCheckBox.Size = new System.Drawing.Size(56, 17);
            this.NTCBetaValueModifyCheckBox.TabIndex = 23;
            this.NTCBetaValueModifyCheckBox.Text = "modify";
            this.NTCBetaValueModifyCheckBox.UseVisualStyleBackColor = true;
            this.NTCBetaValueModifyCheckBox.CheckedChanged += new System.EventHandler(this.NTCBetaValueModifyCheckBox_CheckedChanged);
            // 
            // NTCBetaKelvinLabel
            // 
            this.NTCBetaKelvinLabel.AutoSize = true;
            this.NTCBetaKelvinLabel.Location = new System.Drawing.Point(286, 119);
            this.NTCBetaKelvinLabel.Name = "NTCBetaKelvinLabel";
            this.NTCBetaKelvinLabel.Size = new System.Drawing.Size(14, 13);
            this.NTCBetaKelvinLabel.TabIndex = 22;
            this.NTCBetaKelvinLabel.Text = "K";
            // 
            // NTCBetaValueLabel
            // 
            this.NTCBetaValueLabel.AutoSize = true;
            this.NTCBetaValueLabel.Location = new System.Drawing.Point(162, 119);
            this.NTCBetaValueLabel.Name = "NTCBetaValueLabel";
            this.NTCBetaValueLabel.Size = new System.Drawing.Size(85, 13);
            this.NTCBetaValueLabel.TabIndex = 21;
            this.NTCBetaValueLabel.Text = "NTC beta value:";
            // 
            // NTCBetaValueTextBox
            // 
            this.NTCBetaValueTextBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.NTCBetaValueTextBox.Location = new System.Drawing.Point(248, 115);
            this.NTCBetaValueTextBox.Name = "NTCBetaValueTextBox";
            this.NTCBetaValueTextBox.Size = new System.Drawing.Size(35, 21);
            this.NTCBetaValueTextBox.TabIndex = 20;
            // 
            // WaterPumpLabel
            // 
            this.WaterPumpLabel.AutoSize = true;
            this.WaterPumpLabel.Location = new System.Drawing.Point(6, 49);
            this.WaterPumpLabel.Name = "WaterPumpLabel";
            this.WaterPumpLabel.Size = new System.Drawing.Size(68, 13);
            this.WaterPumpLabel.TabIndex = 19;
            this.WaterPumpLabel.Text = "Water pump:";
            // 
            // DefaultSettingsButton
            // 
            this.DefaultSettingsButton.Location = new System.Drawing.Point(128, 149);
            this.DefaultSettingsButton.Name = "DefaultSettingsButton";
            this.DefaultSettingsButton.Size = new System.Drawing.Size(110, 25);
            this.DefaultSettingsButton.TabIndex = 18;
            this.DefaultSettingsButton.Text = "Default settings";
            this.DefaultSettingsButton.UseVisualStyleBackColor = true;
            this.DefaultSettingsButton.Click += new System.EventHandler(this.DefaultSettingsButton_Click);
            // 
            // WriteSettingsButton
            // 
            this.WriteSettingsButton.Location = new System.Drawing.Point(247, 149);
            this.WriteSettingsButton.Name = "WriteSettingsButton";
            this.WriteSettingsButton.Size = new System.Drawing.Size(110, 25);
            this.WriteSettingsButton.TabIndex = 17;
            this.WriteSettingsButton.Text = "Write settings";
            this.WriteSettingsButton.UseVisualStyleBackColor = true;
            this.WriteSettingsButton.Click += new System.EventHandler(this.WriteSettingsButton_Click);
            // 
            // ReadSettingsButton
            // 
            this.ReadSettingsButton.Location = new System.Drawing.Point(9, 149);
            this.ReadSettingsButton.Name = "ReadSettingsButton";
            this.ReadSettingsButton.Size = new System.Drawing.Size(110, 25);
            this.ReadSettingsButton.TabIndex = 16;
            this.ReadSettingsButton.Text = "Read settings";
            this.ReadSettingsButton.UseVisualStyleBackColor = true;
            this.ReadSettingsButton.Click += new System.EventHandler(this.ReadSettingsButton_Click);
            // 
            // TemperatureSensorsComboBox
            // 
            this.TemperatureSensorsComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.TemperatureSensorsComboBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
            this.TemperatureSensorsComboBox.FormattingEnabled = true;
            this.TemperatureSensorsComboBox.Items.AddRange(new object[] {
            "none",
            "external",
            "internal",
            "ext+int"});
            this.TemperatureSensorsComboBox.Location = new System.Drawing.Point(248, 68);
            this.TemperatureSensorsComboBox.Name = "TemperatureSensorsComboBox";
            this.TemperatureSensorsComboBox.Size = new System.Drawing.Size(80, 21);
            this.TemperatureSensorsComboBox.TabIndex = 12;
            this.TemperatureSensorsComboBox.SelectedIndexChanged += new System.EventHandler(this.TemperatureSensorsComboBox_SelectedIndexChanged);
            // 
            // TemperatureSensorsLabel
            // 
            this.TemperatureSensorsLabel.AutoSize = true;
            this.TemperatureSensorsLabel.Location = new System.Drawing.Point(138, 71);
            this.TemperatureSensorsLabel.Name = "TemperatureSensorsLabel";
            this.TemperatureSensorsLabel.Size = new System.Drawing.Size(109, 13);
            this.TemperatureSensorsLabel.TabIndex = 11;
            this.TemperatureSensorsLabel.Text = "Temperature sensors:";
            // 
            // TemperatureUnitComboBox
            // 
            this.TemperatureUnitComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.TemperatureUnitComboBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
            this.TemperatureUnitComboBox.FormattingEnabled = true;
            this.TemperatureUnitComboBox.Items.AddRange(new object[] {
            "Celsius",
            "Fahrenheit",
            "Kelvin"});
            this.TemperatureUnitComboBox.Location = new System.Drawing.Point(248, 91);
            this.TemperatureUnitComboBox.Name = "TemperatureUnitComboBox";
            this.TemperatureUnitComboBox.Size = new System.Drawing.Size(80, 21);
            this.TemperatureUnitComboBox.TabIndex = 10;
            // 
            // TemperatureUnitLabel
            // 
            this.TemperatureUnitLabel.AutoSize = true;
            this.TemperatureUnitLabel.Location = new System.Drawing.Point(157, 94);
            this.TemperatureUnitLabel.Name = "TemperatureUnitLabel";
            this.TemperatureUnitLabel.Size = new System.Drawing.Size(90, 13);
            this.TemperatureUnitLabel.TabIndex = 9;
            this.TemperatureUnitLabel.Text = "Temperature unit:";
            // 
            // MinutesLabel02
            // 
            this.MinutesLabel02.AutoSize = true;
            this.MinutesLabel02.Location = new System.Drawing.Point(98, 95);
            this.MinutesLabel02.Name = "MinutesLabel02";
            this.MinutesLabel02.Size = new System.Drawing.Size(23, 13);
            this.MinutesLabel02.TabIndex = 8;
            this.MinutesLabel02.Text = "min";
            // 
            // WaterPumpOffTimeLabel
            // 
            this.WaterPumpOffTimeLabel.AutoSize = true;
            this.WaterPumpOffTimeLabel.Location = new System.Drawing.Point(6, 95);
            this.WaterPumpOffTimeLabel.Name = "WaterPumpOffTimeLabel";
            this.WaterPumpOffTimeLabel.Size = new System.Drawing.Size(50, 13);
            this.WaterPumpOffTimeLabel.TabIndex = 7;
            this.WaterPumpOffTimeLabel.Text = "- off-time:";
            // 
            // WaterPumpOffTimeTextBox
            // 
            this.WaterPumpOffTimeTextBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.WaterPumpOffTimeTextBox.Location = new System.Drawing.Point(60, 91);
            this.WaterPumpOffTimeTextBox.Name = "WaterPumpOffTimeTextBox";
            this.WaterPumpOffTimeTextBox.Size = new System.Drawing.Size(35, 21);
            this.WaterPumpOffTimeTextBox.TabIndex = 6;
            // 
            // MinutesLabel01
            // 
            this.MinutesLabel01.AutoSize = true;
            this.MinutesLabel01.Location = new System.Drawing.Point(98, 71);
            this.MinutesLabel01.Name = "MinutesLabel01";
            this.MinutesLabel01.Size = new System.Drawing.Size(23, 13);
            this.MinutesLabel01.TabIndex = 5;
            this.MinutesLabel01.Text = "min";
            // 
            // WaterPumpOnTimeTextBox
            // 
            this.WaterPumpOnTimeTextBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.WaterPumpOnTimeTextBox.Location = new System.Drawing.Point(60, 68);
            this.WaterPumpOnTimeTextBox.Name = "WaterPumpOnTimeTextBox";
            this.WaterPumpOnTimeTextBox.Size = new System.Drawing.Size(35, 21);
            this.WaterPumpOnTimeTextBox.TabIndex = 4;
            // 
            // WaterPumpOnTimeLabel
            // 
            this.WaterPumpOnTimeLabel.AutoSize = true;
            this.WaterPumpOnTimeLabel.Location = new System.Drawing.Point(6, 72);
            this.WaterPumpOnTimeLabel.Name = "WaterPumpOnTimeLabel";
            this.WaterPumpOnTimeLabel.Size = new System.Drawing.Size(50, 13);
            this.WaterPumpOnTimeLabel.TabIndex = 3;
            this.WaterPumpOnTimeLabel.Text = "- on-time:";
            // 
            // WaterPumpSpeedPercentageLabel
            // 
            this.WaterPumpSpeedPercentageLabel.AutoSize = true;
            this.WaterPumpSpeedPercentageLabel.Location = new System.Drawing.Point(328, 27);
            this.WaterPumpSpeedPercentageLabel.Name = "WaterPumpSpeedPercentageLabel";
            this.WaterPumpSpeedPercentageLabel.Size = new System.Drawing.Size(27, 13);
            this.WaterPumpSpeedPercentageLabel.TabIndex = 2;
            this.WaterPumpSpeedPercentageLabel.Text = "50%";
            // 
            // WaterPumpSpeedLabel
            // 
            this.WaterPumpSpeedLabel.AutoSize = true;
            this.WaterPumpSpeedLabel.Location = new System.Drawing.Point(6, 26);
            this.WaterPumpSpeedLabel.Name = "WaterPumpSpeedLabel";
            this.WaterPumpSpeedLabel.Size = new System.Drawing.Size(100, 13);
            this.WaterPumpSpeedLabel.TabIndex = 1;
            this.WaterPumpSpeedLabel.Text = "Water pump speed:";
            // 
            // WaterPumpSpeedTrackBar
            // 
            this.WaterPumpSpeedTrackBar.Location = new System.Drawing.Point(107, 13);
            this.WaterPumpSpeedTrackBar.Maximum = 100;
            this.WaterPumpSpeedTrackBar.Name = "WaterPumpSpeedTrackBar";
            this.WaterPumpSpeedTrackBar.Size = new System.Drawing.Size(225, 45);
            this.WaterPumpSpeedTrackBar.TabIndex = 0;
            this.WaterPumpSpeedTrackBar.TickFrequency = 5;
            this.WaterPumpSpeedTrackBar.TickStyle = System.Windows.Forms.TickStyle.Both;
            this.WaterPumpSpeedTrackBar.Value = 50;
            this.WaterPumpSpeedTrackBar.Scroll += new System.EventHandler(this.WaterPumpSpeedTrackBar_Scroll);
            // 
            // ExternalTemperatureTextBox
            // 
            this.ExternalTemperatureTextBox.BackColor = System.Drawing.SystemColors.Window;
            this.ExternalTemperatureTextBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.ExternalTemperatureTextBox.Location = new System.Drawing.Point(57, 17);
            this.ExternalTemperatureTextBox.Name = "ExternalTemperatureTextBox";
            this.ExternalTemperatureTextBox.ReadOnly = true;
            this.ExternalTemperatureTextBox.Size = new System.Drawing.Size(41, 21);
            this.ExternalTemperatureTextBox.TabIndex = 15;
            // 
            // InternalTemperatureTextBox
            // 
            this.InternalTemperatureTextBox.BackColor = System.Drawing.SystemColors.Window;
            this.InternalTemperatureTextBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.InternalTemperatureTextBox.Location = new System.Drawing.Point(174, 17);
            this.InternalTemperatureTextBox.Name = "InternalTemperatureTextBox";
            this.InternalTemperatureTextBox.ReadOnly = true;
            this.InternalTemperatureTextBox.Size = new System.Drawing.Size(41, 21);
            this.InternalTemperatureTextBox.TabIndex = 14;
            // 
            // ControlGroupBox
            // 
            this.ControlGroupBox.Controls.Add(this.StatusButton);
            this.ControlGroupBox.Controls.Add(this.ResetButton);
            this.ControlGroupBox.Controls.Add(this.RefreshButton);
            this.ControlGroupBox.Controls.Add(this.COMPortsComboBox);
            this.ControlGroupBox.Controls.Add(this.ConnectButton);
            this.ControlGroupBox.Location = new System.Drawing.Point(12, 542);
            this.ControlGroupBox.Name = "ControlGroupBox";
            this.ControlGroupBox.Size = new System.Drawing.Size(365, 57);
            this.ControlGroupBox.TabIndex = 3;
            this.ControlGroupBox.TabStop = false;
            this.ControlGroupBox.Text = "Control";
            // 
            // StatusButton
            // 
            this.StatusButton.Enabled = false;
            this.StatusButton.Location = new System.Drawing.Point(251, 19);
            this.StatusButton.Name = "StatusButton";
            this.StatusButton.Size = new System.Drawing.Size(50, 25);
            this.StatusButton.TabIndex = 4;
            this.StatusButton.Text = "Status";
            this.StatusButton.UseVisualStyleBackColor = true;
            this.StatusButton.Click += new System.EventHandler(this.StatusButton_Click);
            // 
            // ResetButton
            // 
            this.ResetButton.Enabled = false;
            this.ResetButton.Location = new System.Drawing.Point(306, 19);
            this.ResetButton.Name = "ResetButton";
            this.ResetButton.Size = new System.Drawing.Size(50, 25);
            this.ResetButton.TabIndex = 3;
            this.ResetButton.Text = "Reset";
            this.ResetButton.UseVisualStyleBackColor = true;
            this.ResetButton.Click += new System.EventHandler(this.ResetButton_Click);
            // 
            // RefreshButton
            // 
            this.RefreshButton.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
            this.RefreshButton.Location = new System.Drawing.Point(156, 19);
            this.RefreshButton.Name = "RefreshButton";
            this.RefreshButton.Size = new System.Drawing.Size(55, 25);
            this.RefreshButton.TabIndex = 2;
            this.RefreshButton.Text = "Refresh";
            this.RefreshButton.UseVisualStyleBackColor = true;
            this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
            // 
            // COMPortsComboBox
            // 
            this.COMPortsComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.COMPortsComboBox.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.COMPortsComboBox.FormattingEnabled = true;
            this.COMPortsComboBox.Location = new System.Drawing.Point(90, 20);
            this.COMPortsComboBox.Name = "COMPortsComboBox";
            this.COMPortsComboBox.Size = new System.Drawing.Size(60, 23);
            this.COMPortsComboBox.TabIndex = 1;
            // 
            // ConnectButton
            // 
            this.ConnectButton.Location = new System.Drawing.Point(9, 19);
            this.ConnectButton.Name = "ConnectButton";
            this.ConnectButton.Size = new System.Drawing.Size(75, 25);
            this.ConnectButton.TabIndex = 0;
            this.ConnectButton.Text = "Connect";
            this.ConnectButton.UseVisualStyleBackColor = true;
            this.ConnectButton.Click += new System.EventHandler(this.ConnectButton_Click);
            // 
            // ExternalTemperatureLabel
            // 
            this.ExternalTemperatureLabel.AutoSize = true;
            this.ExternalTemperatureLabel.Location = new System.Drawing.Point(8, 21);
            this.ExternalTemperatureLabel.Name = "ExternalTemperatureLabel";
            this.ExternalTemperatureLabel.Size = new System.Drawing.Size(48, 13);
            this.ExternalTemperatureLabel.TabIndex = 20;
            this.ExternalTemperatureLabel.Text = "External:";
            // 
            // InternalTemperatureLabel
            // 
            this.InternalTemperatureLabel.AutoSize = true;
            this.InternalTemperatureLabel.Location = new System.Drawing.Point(128, 21);
            this.InternalTemperatureLabel.Name = "InternalTemperatureLabel";
            this.InternalTemperatureLabel.Size = new System.Drawing.Size(45, 13);
            this.InternalTemperatureLabel.TabIndex = 21;
            this.InternalTemperatureLabel.Text = "Internal:";
            // 
            // TemperatureUnitLabel01
            // 
            this.TemperatureUnitLabel01.AutoSize = true;
            this.TemperatureUnitLabel01.Location = new System.Drawing.Point(99, 21);
            this.TemperatureUnitLabel01.Name = "TemperatureUnitLabel01";
            this.TemperatureUnitLabel01.Size = new System.Drawing.Size(10, 13);
            this.TemperatureUnitLabel01.TabIndex = 22;
            this.TemperatureUnitLabel01.Text = " ";
            // 
            // TemperatureUnitLabel02
            // 
            this.TemperatureUnitLabel02.AutoSize = true;
            this.TemperatureUnitLabel02.Location = new System.Drawing.Point(216, 21);
            this.TemperatureUnitLabel02.Name = "TemperatureUnitLabel02";
            this.TemperatureUnitLabel02.Size = new System.Drawing.Size(10, 13);
            this.TemperatureUnitLabel02.TabIndex = 23;
            this.TemperatureUnitLabel02.Text = " ";
            // 
            // TemperaturesGroupBox
            // 
            this.TemperaturesGroupBox.Controls.Add(this.InternalTemperatureLabel);
            this.TemperaturesGroupBox.Controls.Add(this.TemperatureUnitLabel01);
            this.TemperaturesGroupBox.Controls.Add(this.InternalTemperatureTextBox);
            this.TemperaturesGroupBox.Controls.Add(this.ExternalTemperatureLabel);
            this.TemperaturesGroupBox.Controls.Add(this.TemperatureUnitLabel02);
            this.TemperaturesGroupBox.Controls.Add(this.ExternalTemperatureTextBox);
            this.TemperaturesGroupBox.Location = new System.Drawing.Point(12, 486);
            this.TemperaturesGroupBox.Name = "TemperaturesGroupBox";
            this.TemperaturesGroupBox.Size = new System.Drawing.Size(365, 50);
            this.TemperaturesGroupBox.TabIndex = 24;
            this.TemperaturesGroupBox.TabStop = false;
            this.TemperaturesGroupBox.Text = "Temperatures";
            // 
            // DebugGroupBox
            // 
            this.DebugGroupBox.Controls.Add(this.WaterPumpOnOffCheckBox);
            this.DebugGroupBox.Controls.Add(this.GetStatusUpdatesCheckBox);
            this.DebugGroupBox.Location = new System.Drawing.Point(388, 27);
            this.DebugGroupBox.Name = "DebugGroupBox";
            this.DebugGroupBox.Size = new System.Drawing.Size(365, 572);
            this.DebugGroupBox.TabIndex = 25;
            this.DebugGroupBox.TabStop = false;
            this.DebugGroupBox.Text = "Debug";
            // 
            // WaterPumpOnOffCheckBox
            // 
            this.WaterPumpOnOffCheckBox.AutoSize = true;
            this.WaterPumpOnOffCheckBox.Checked = true;
            this.WaterPumpOnOffCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.WaterPumpOnOffCheckBox.Location = new System.Drawing.Point(6, 528);
            this.WaterPumpOnOffCheckBox.Name = "WaterPumpOnOffCheckBox";
            this.WaterPumpOnOffCheckBox.Size = new System.Drawing.Size(290, 17);
            this.WaterPumpOnOffCheckBox.TabIndex = 1;
            this.WaterPumpOnOffCheckBox.Text = "Water pump on/off (only when service mode is enabled)";
            this.WaterPumpOnOffCheckBox.UseVisualStyleBackColor = true;
            this.WaterPumpOnOffCheckBox.CheckedChanged += new System.EventHandler(this.WaterPumpOnOffCheckBox_CheckedChanged);
            // 
            // GetStatusUpdatesCheckBox
            // 
            this.GetStatusUpdatesCheckBox.AutoSize = true;
            this.GetStatusUpdatesCheckBox.Checked = true;
            this.GetStatusUpdatesCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.GetStatusUpdatesCheckBox.Location = new System.Drawing.Point(6, 551);
            this.GetStatusUpdatesCheckBox.Name = "GetStatusUpdatesCheckBox";
            this.GetStatusUpdatesCheckBox.Size = new System.Drawing.Size(226, 17);
            this.GetStatusUpdatesCheckBox.TabIndex = 0;
            this.GetStatusUpdatesCheckBox.Text = "Get regular status updates from the device";
            this.GetStatusUpdatesCheckBox.UseVisualStyleBackColor = true;
            this.GetStatusUpdatesCheckBox.CheckedChanged += new System.EventHandler(this.GetStatusUpdatesCheckBox_CheckedChanged);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(764, 611);
            this.Controls.Add(this.DebugGroupBox);
            this.Controls.Add(this.TemperaturesGroupBox);
            this.Controls.Add(this.ControlGroupBox);
            this.Controls.Add(this.SettingsGroupBox);
            this.Controls.Add(this.CommunicationGroupBox);
            this.Controls.Add(this.MenuStrip);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MainMenuStrip = this.MenuStrip;
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.Text = "Modular Hydroponic Tower Garden";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.MenuStrip.ResumeLayout(false);
            this.MenuStrip.PerformLayout();
            this.CommunicationGroupBox.ResumeLayout(false);
            this.CommunicationGroupBox.PerformLayout();
            this.SettingsGroupBox.ResumeLayout(false);
            this.SettingsGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.WaterPumpSpeedTrackBar)).EndInit();
            this.ControlGroupBox.ResumeLayout(false);
            this.TemperaturesGroupBox.ResumeLayout(false);
            this.TemperaturesGroupBox.PerformLayout();
            this.DebugGroupBox.ResumeLayout(false);
            this.DebugGroupBox.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip MenuStrip;
        private System.Windows.Forms.ToolStripMenuItem AboutToolStripMenuItem;
        private System.Windows.Forms.GroupBox CommunicationGroupBox;
        private System.Windows.Forms.TextBox CommunicationTextBox;
        private System.Windows.Forms.Button SendButton;
        private System.Windows.Forms.ComboBox SendComboBox;
        private System.Windows.Forms.GroupBox SettingsGroupBox;
        private System.Windows.Forms.TrackBar WaterPumpSpeedTrackBar;
        private System.Windows.Forms.Label WaterPumpSpeedPercentageLabel;
        private System.Windows.Forms.Label WaterPumpSpeedLabel;
        private System.Windows.Forms.TextBox WaterPumpOnTimeTextBox;
        private System.Windows.Forms.Label WaterPumpOnTimeLabel;
        private System.Windows.Forms.Label MinutesLabel02;
        private System.Windows.Forms.Label WaterPumpOffTimeLabel;
        private System.Windows.Forms.TextBox WaterPumpOffTimeTextBox;
        private System.Windows.Forms.Label MinutesLabel01;
        private System.Windows.Forms.GroupBox ControlGroupBox;
        private System.Windows.Forms.Button RefreshButton;
        private System.Windows.Forms.ComboBox COMPortsComboBox;
        private System.Windows.Forms.Button ConnectButton;
        private System.Windows.Forms.ComboBox TemperatureUnitComboBox;
        private System.Windows.Forms.Label TemperatureUnitLabel;
        private System.Windows.Forms.ComboBox TemperatureSensorsComboBox;
        private System.Windows.Forms.Label TemperatureSensorsLabel;
        private System.Windows.Forms.TextBox InternalTemperatureTextBox;
        private System.Windows.Forms.TextBox ExternalTemperatureTextBox;
        private System.Windows.Forms.Button DefaultSettingsButton;
        private System.Windows.Forms.Button WriteSettingsButton;
        private System.Windows.Forms.Button ReadSettingsButton;
        private System.Windows.Forms.Button ResetButton;
        private System.Windows.Forms.Label TemperatureUnitLabel02;
        private System.Windows.Forms.Label TemperatureUnitLabel01;
        private System.Windows.Forms.Label InternalTemperatureLabel;
        private System.Windows.Forms.Label ExternalTemperatureLabel;
        private System.Windows.Forms.Label WaterPumpLabel;
        private System.Windows.Forms.GroupBox TemperaturesGroupBox;
        private System.Windows.Forms.Label NTCBetaKelvinLabel;
        private System.Windows.Forms.Label NTCBetaValueLabel;
        private System.Windows.Forms.TextBox NTCBetaValueTextBox;
        private System.Windows.Forms.CheckBox NTCBetaValueModifyCheckBox;
        private System.Windows.Forms.Button StatusButton;
        private System.Windows.Forms.ToolStripMenuItem ToolsToolStripMenuItem;
        private System.Windows.Forms.GroupBox DebugGroupBox;
        private System.Windows.Forms.ToolStripMenuItem UpdateFirmwareToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem ShowDebugToolsToolStripMenuItem;
        private System.Windows.Forms.CheckBox GetStatusUpdatesCheckBox;
        private System.Windows.Forms.ToolStripMenuItem ServiceModeToolStripMenuItem;
        private System.Windows.Forms.CheckBox WaterPumpOnOffCheckBox;
    }
}

