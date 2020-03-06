using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading;
using System.Timers;
using System.Windows.Forms;

namespace MHTG
{
    public partial class MainForm : Form
    {
        public bool SerialPortAvailable = false;
        public bool Timeout = false;
        public bool DeviceFound = false;
        public List<byte> bufferlist = new List<byte>();

        public byte WaterPumpSpeed = 0;
        public uint WaterPumpOnTime = 0;
        public uint WaterPumpOffTime = 0;
        public byte TemperatureUnit = 0;
        public int NTCBetaValue = 0;
        public byte TemperatureSensors = 0;
        public float ExternalTemperature = 0;
        public float InternalTemperature = 0;

        public string SelectedPort = String.Empty;
        public static string UpdatePort = String.Empty;
        public static UInt64 OldFWUNIXTime = 0;
        public static UInt64 NewFWUNIXTime = 0;

        public string DateTimeNow;
        public static string USBLogFilename;
        public static string USBBinaryLogFilename;

        public bool WaterPumpState = false;
        public bool ServiceMode = false;

        public byte[] ResetDevice = new byte[] { 0x3D, 0x00, 0x02, 0x00, 0x00, 0x02 };
        public byte[] HandshakeRequest = new byte[] { 0x3D, 0x00, 0x02, 0x01, 0x01, 0x04 };
        public byte[] ExpectedHandshake = new byte[] { 0x3D, 0x00, 0x06, 0x81, 0x00, 0x4D, 0x48, 0x54, 0x47, 0xB7 };
        public byte[] StatusRequest = new byte[] { 0x3D, 0x00, 0x02, 0x02, 0x00, 0x04 };
        public byte[] EEPROMDumpRequest = new byte[] { 0x3D, 0x00, 0x06, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x20, 0x35 }; // read full EEPROM (1024 bytes)
        public byte[] ReadSettings = new byte[] { 0x3D, 0x00, 0x02, 0x03, 0x01, 0x06 };
        public byte[] HwFwInfoRequest = new byte[] { 0x3D, 0x00, 0x02, 0x04, 0x02, 0x08 };
        public byte[] TemperatureRequest = new byte[] { 0x3D, 0x00, 0x02, 0x04, 0x04, 0x0A };
        public byte[] GetStatusUpdatesOn = new byte[] { 0x3D, 0x00, 0x03, 0x03, 0x03, 0x01, 0x0A };
        public byte[] GetStatusUpdatesOff = new byte[] { 0x3D, 0x00, 0x03, 0x03, 0x03, 0x00, 0x09 };
        public byte[] ServiceModeOn = new byte[] { 0x3D, 0x00, 0x03, 0x03, 0x04, 0x01, 0x0B };
        public byte[] ServiceModeOff = new byte[] { 0x3D, 0x00, 0x03, 0x03, 0x04, 0x00, 0x0A };
        public byte[] TurnOnWaterPump = new byte[] { 0x3D, 0x00, 0x03, 0x03, 0x05, 0x00, 0x0B };
        public byte[] TurnOffWaterPump = new byte[] { 0x3D, 0x00, 0x03, 0x03, 0x05, 0x00, 0x0B };

        AboutForm about;
        SerialPort Serial = new SerialPort();
        System.Timers.Timer TimeoutTimer = new System.Timers.Timer();
        WebClient Downloader = new WebClient();
        Uri FlashFile = new Uri("https://github.com/laszlodaniel/ModularHydroponicTowerGarden/raw/master/Arduino/MHTG/MHTG.ino.standard.hex");
        Uri SourceFile = new Uri("https://github.com/laszlodaniel/ModularHydroponicTowerGarden/raw/master/Arduino/MHTG/MHTG.ino");

        public MainForm()
        {
            InitializeComponent();
            Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath);
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            DebugGroupBox.Visible = false;
            this.Size = new Size(405, 650); // resize form to collapsed view
            this.CenterToScreen(); // put window at the center of the screen

            // Create LOG directory if it doesn't exist
            if (!Directory.Exists("LOG")) Directory.CreateDirectory("LOG");

            // Set logfile names inside the LOG directory
            DateTimeNow = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            USBLogFilename = @"LOG/usblog_" + DateTimeNow + ".txt";
            USBBinaryLogFilename = @"LOG/usblog_" + DateTimeNow + ".bin";

            UpdateCOMPortList();

            // Setup timeout timer
            TimeoutTimer.Elapsed += new ElapsedEventHandler(TimeoutHandler);
            TimeoutTimer.Interval = 2000; // ms
            TimeoutTimer.Enabled = false;

            SettingsGroupBox.Enabled = false;
            TemperaturesGroupBox.Enabled = false;
            NTCBetaValueModifyCheckBox_CheckedChanged(this, EventArgs.Empty);
            SendComboBox.Items.Clear();

            ActiveControl = ConnectButton; // put focus on the connect button
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            Application.DoEvents();
            if (DeviceFound) ConnectButton.PerformClick(); // disconnect first
            if (Serial.IsOpen) Serial.Close();
        }

        private void TimeoutHandler(object source, ElapsedEventArgs e)
        {
            Timeout = true;
        }

        private void UpdateCOMPortList()
        {
            COMPortsComboBox.Items.Clear(); // clear combobox
            string[] ports = SerialPort.GetPortNames(); // get available ports

            if (ports.Length > 0)
            {
                COMPortsComboBox.Items.AddRange(ports);
                SerialPortAvailable = true;
                ConnectButton.Enabled = true;

                if (SelectedPort == String.Empty) // if no port has been selected
                {
                    COMPortsComboBox.SelectedIndex = 0; // select first available port
                    SelectedPort = COMPortsComboBox.Text;
                }
                else
                {
                    try
                    {
                        COMPortsComboBox.SelectedIndex = COMPortsComboBox.Items.IndexOf(SelectedPort);
                    }
                    catch
                    {
                        COMPortsComboBox.SelectedIndex = 0;
                    }
                }
            }
            else
            {
                COMPortsComboBox.Items.Add("N/A");
                SerialPortAvailable = false;
                ConnectButton.Enabled = false;
                COMPortsComboBox.SelectedIndex = 0; // select "N/A"
                SelectedPort = String.Empty;
                Util.UpdateTextBox(CommunicationTextBox, "[INFO] No device available", null);
            }
        }

        private void COMPortsComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            SelectedPort = COMPortsComboBox.Text;
        }

        private void RefreshButton_Click(object sender, EventArgs e)
        {
            UpdateCOMPortList();
        }

        private void WaterPumpSpeedTrackBar_Scroll(object sender, EventArgs e)
        {
            WaterPumpSpeedPercentageLabel.Text = WaterPumpSpeedTrackBar.Value.ToString() + "%";
        }

        private void ConnectButton_Click(object sender, EventArgs e)
        {
            if (!DeviceFound) // connect
            {
                UpdateCOMPortList();

                if (SerialPortAvailable)
                {
                    byte[] buffer = new byte[2048];
                    byte ConnectionCounter = 0;

                    while (ConnectionCounter < 5) // try connecting to the device 5 times, then give up
                    {
                        ConnectButton.Enabled = false; // no double-click

                        if (Serial.IsOpen) Serial.Close(); // can't overwrite fields if serial port is open
                        Serial.PortName = COMPortsComboBox.Text;
                        Serial.BaudRate = 250000;
                        Serial.DataBits = 8;
                        Serial.StopBits = StopBits.One;
                        Serial.Parity = Parity.None;
                        Serial.ReadTimeout = 500;
                        Serial.WriteTimeout = 500;

                        Util.UpdateTextBox(CommunicationTextBox, "[INFO] Connecting to " + Serial.PortName, null);

                        try
                        {
                            Serial.Open(); // open current serial port
                        }
                        catch
                        {
                            Util.UpdateTextBox(CommunicationTextBox, "[INFO] " + Serial.PortName + " is opened by another application", null);
                            Util.UpdateTextBox(CommunicationTextBox, "[INFO] Device not found at " + Serial.PortName, null);
                            break;
                        }

                        if (Serial.IsOpen)
                        {
                            Serial.DiscardInBuffer();
                            Serial.DiscardOutBuffer();
                            Serial.BaseStream.Flush();

                            Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Handshake request (" + Serial.PortName + ")", HandshakeRequest);
                            Serial.Write(HandshakeRequest, 0, HandshakeRequest.Length);

                            Timeout = false;
                            TimeoutTimer.Enabled = true;

                            while (!Timeout)
                            {
                                if (Serial.BytesToRead > 9)
                                {
                                    Serial.Read(buffer, 0, 10);
                                    break;
                                }
                            }

                            TimeoutTimer.Enabled = false;

                            Serial.DiscardInBuffer();
                            Serial.DiscardOutBuffer();
                            Serial.BaseStream.Flush();

                            if (Timeout)
                            {
                                Util.UpdateTextBox(CommunicationTextBox, "[INFO] Device is not responding at " + Serial.PortName, null);
                                Timeout = false;
                                Serial.Close();
                                ConnectionCounter++; // increase counter value and try again
                            }
                            else
                            {
                                DeviceFound = Util.CompareArrays(buffer, ExpectedHandshake, 0, ExpectedHandshake.Length);

                                if (DeviceFound)
                                {
                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Handshake response", ExpectedHandshake);
                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Handshake OK: MHTG", null);
                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Device connected (" + Serial.PortName + ")", null);
                                    UpdatePort = Serial.PortName;
                                    ConnectButton.Text = "Disconnect";
                                    SettingsGroupBox.Enabled = true;
                                    TemperaturesGroupBox.Enabled = true;
                                    StatusButton.Enabled = true;
                                    ResetButton.Enabled = true;
                                    UpdateFirmwareToolStripMenuItem.Enabled = true;
                                    ServiceModeToolStripMenuItem.Enabled = true;
                                    Serial.DataReceived += new SerialDataReceivedEventHandler(SerialDataReceivedHandler);
                                    Serial.Write(HwFwInfoRequest, 0, HwFwInfoRequest.Length); // send a bunch of requests
                                    Serial.Write(ReadSettings, 0, ReadSettings.Length);
                                    Serial.Write(StatusRequest, 0, StatusRequest.Length);
                                    break; // exit while-loop
                                }
                                else
                                {
                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", buffer.Take(10).ToArray());
                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Handshake ERROR: " + Encoding.ASCII.GetString(buffer, 5, 4), null);
                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Device not found at " + Serial.PortName, null);
                                    Serial.Close();
                                    ConnectionCounter++; // increase counter value and try again
                                }
                            }
                        }
                    }

                    ConnectButton.Enabled = true;
                }
            }
            else // disconnect
            {
                if (Serial.IsOpen)
                {
                    Serial.DiscardInBuffer();
                    Serial.DiscardOutBuffer();
                    Serial.BaseStream.Flush();
                    Serial.Close();
                    DeviceFound = false;
                    ConnectButton.Text = "Connect";
                    SettingsGroupBox.Enabled = false;
                    TemperaturesGroupBox.Enabled = false;
                    StatusButton.Enabled = false;
                    ResetButton.Enabled = false;
                    UpdateFirmwareToolStripMenuItem.Enabled = false;
                    ServiceModeToolStripMenuItem.Enabled = false;
                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Device disconnected (" + Serial.PortName + ")", null);
                    Serial.DataReceived -= new SerialDataReceivedEventHandler(SerialDataReceivedHandler);
                }
            }
        }

        private void SerialDataReceivedHandler(object sender, SerialDataReceivedEventArgs e)
        {
            SerialPort sp = (SerialPort)sender;
            int DataLength = sp.BytesToRead;

            // This approach enables reading multiple broken transmissions
            for (int i = 0; i < DataLength; i++)
            {
                try
                {
                    bufferlist.Add((byte)sp.ReadByte());
                }
                catch
                {
                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Serial read error", null);
                    break;
                }
            }

            // Multiple packets are handled one after another in this while-loop
            while (bufferlist.Count > 0)
            {
                if (bufferlist[0] == 0x3D)
                {
                    if (bufferlist.Count < 3) break; // wait for the length bytes
                    
                    int PacketLength = (bufferlist[1] << 8) + bufferlist[2];
                    int FullPacketLength = PacketLength + 4;

                    if (bufferlist.Count < FullPacketLength) break; // wait for the rest of the bytes to arrive

                    byte[] packet = new byte[FullPacketLength];
                    int PayloadLength = PacketLength - 2;
                    byte[] Payload = new byte[PayloadLength];
                    int ChecksumLocation = PacketLength + 3;
                    byte DataCode = 0;
                    byte Source = 0;
                    byte Command = 0;
                    byte SubDataCode = 0;
                    byte Checksum = 0;
                    byte CalculatedChecksum = 0;
                    Array.Copy(bufferlist.ToArray(), 0, packet, 0, packet.Length);

                    Checksum = packet[ChecksumLocation]; // get packet checksum byte

                    for (int i = 1; i < ChecksumLocation; i++)
                    {
                        CalculatedChecksum += packet[i]; // calculate checksum
                    }

                    if (CalculatedChecksum == Checksum) // verify checksum
                    {
                        DataCode = packet[3];
                        Source = (byte)((DataCode >> 7) & 0x01);
                        Command = (byte)(DataCode & 0x0F);
                        SubDataCode = packet[4];

                        if (Source == 1) // highest bit set in the DataCode byte means the packet is coming from the device
                        {
                            if (PayloadLength > 0) // copy payload bytes if available
                            {
                                Array.Copy(packet, 5, Payload, 0, Payload.Length);
                            }

                            switch (Command) // based on the datacode decide what to do with this packet
                            {
                                case 0x00: // Reset
                                    switch (SubDataCode)
                                    {
                                        case 0x00:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Device is resetting", packet);
                                            break;
                                        case 0x01:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Device is ready", packet);
                                            break;
                                        default:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Unknown reset packet", packet);
                                            break;
                                    }
                                    break;
                                case 0x01: // Handshake
                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Handshake received", packet);
                                    if (Encoding.ASCII.GetString(Payload, 0, Payload.Length) == "MHTG") Util.UpdateTextBox(CommunicationTextBox, "[INFO] Handshake OK: MHTG", null);
                                    else Util.UpdateTextBox(CommunicationTextBox, "[INFO] Handshake ERROR: " + Encoding.ASCII.GetString(Payload, 0, Payload.Length), null);
                                    break;
                                case 0x02: // Status
                                    if (Payload.Length > 13)
                                    {
                                        string WaterPumpStateString = string.Empty;
                                        byte WaterPumpPercentValue = (byte)(Math.Round((float)(Payload[1] + 1) / 256 * 100));
                                        string WaterPumpPercentLevel = WaterPumpPercentValue.ToString("0") + "%";
                                        string ServiceModeString = string.Empty;
                                        TimeSpan RemainingTimeMillis = TimeSpan.FromMilliseconds(Payload[2] << 24 | Payload[3] << 16 | Payload[4] << 8 | Payload[5]);
                                        DateTime RemainingTime = DateTime.Today.Add(RemainingTimeMillis);
                                        string RemainingTimeString = RemainingTime.ToString("HH:mm:ss");

                                        if ((Payload[0] & 0x01) == 0x01) WaterPumpState = true; // enabled
                                        else WaterPumpState = false; // disabled

                                        if (WaterPumpState)
                                        {
                                            if (WaterPumpOnTime > 0) WaterPumpStateString = "on @ " + WaterPumpPercentLevel;
                                            else WaterPumpStateString = "off";
                                        }
                                        else
                                        {
                                            WaterPumpStateString = "off";
                                            WaterPumpState = false;
                                        }

                                        if (((Payload[0] >> 1) & 0x01) == 0x01) ServiceMode = true;
                                        else ServiceMode = false;

                                        if (ServiceMode)
                                        {
                                            ServiceModeString = "enabled";
                                            WaterPumpStateString = "off";
                                        }
                                        else ServiceModeString = "disabled";

                                        ExternalTemperature = BitConverter.ToSingle(new byte[4] { Payload[6], Payload[7], Payload[8], Payload[9] }, 0);
                                        InternalTemperature = BitConverter.ToSingle(new byte[4] { Payload[10], Payload[11], Payload[12], Payload[13] }, 0);
                                        string TemperatureString = "         - external: ---.-" + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                   "         - internal: ---.-" + TemperatureUnitLabel02.Text;

                                        TemperaturesGroupBox.BeginInvoke((MethodInvoker)delegate
                                        {
                                            switch (TemperatureSensors)
                                            {
                                                case 0: // none
                                                TemperatureString = "         - external: ---.-" + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                        "         - internal: ---.-" + TemperatureUnitLabel02.Text;
                                                    ExternalTemperatureTextBox.Text = "---.-";
                                                    InternalTemperatureTextBox.Text = "---.-";
                                                    break;
                                                case 1: // external
                                                TemperatureString = "         - external: " + ExternalTemperature.ToString("0.0") + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                        "         - internal: ---.-" + TemperatureUnitLabel02.Text;
                                                    ExternalTemperatureTextBox.Text = ExternalTemperature.ToString("0.0");
                                                    InternalTemperatureTextBox.Text = "---.-";
                                                    break;
                                                case 2: // internal
                                                TemperatureString = "         - external: ---.-" + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                        "         - internal: " + InternalTemperature.ToString("0.0") + TemperatureUnitLabel02.Text;
                                                    ExternalTemperatureTextBox.Text = "---.-";
                                                    InternalTemperatureTextBox.Text = InternalTemperature.ToString("0.0");
                                                    break;
                                                case 3: // external and internal (both)
                                                TemperatureString = "         - external: " + ExternalTemperature.ToString("0.0") + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                        "         - internal: " + InternalTemperature.ToString("0.0") + TemperatureUnitLabel02.Text;
                                                    ExternalTemperatureTextBox.Text = ExternalTemperature.ToString("0.0");
                                                    InternalTemperatureTextBox.Text = InternalTemperature.ToString("0.0");
                                                    break;
                                                default:
                                                    TemperatureString = "         - external: ---.-" + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                        "         - internal: ---.-" + TemperatureUnitLabel02.Text;
                                                    ExternalTemperatureTextBox.Text = "---.-";
                                                    InternalTemperatureTextBox.Text = "---.-";
                                                    break;
                                            }
                                        });

                                        Util.UpdateTextBox(CommunicationTextBox, "[RX->] Status response", packet);
                                        Util.UpdateTextBox(CommunicationTextBox, "[INFO] Current status: " + Environment.NewLine +
                                                                                 "       - service mode: " + ServiceModeString + Environment.NewLine +
                                                                                 "       - water pump state: " + WaterPumpStateString + Environment.NewLine +
                                                                                 "       - remaining time: " + RemainingTimeString + Environment.NewLine +
                                                                                 "       - temperatures: " + Environment.NewLine +
                                                                                 TemperatureString, null);
                                    }
                                    else
                                    {
                                        Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                    }
                                    break;
                                case 0x03: // Settings
                                    switch (SubDataCode)
                                    {
                                        case 0x01: // read settings
                                            if (PayloadLength > 12)
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Settings response", packet);
                                                WaterPumpSpeed = (byte)(Math.Round((float)(Payload[0] + 1) / 256 * 100)); // convert PWM value to percent
                                                WaterPumpOnTime = (uint)((Payload[1] << 24) | (Payload[2] << 16) | (Payload[3] << 8) | Payload[4]);
                                                WaterPumpOffTime = (uint)((Payload[5] << 24) | (Payload[6] << 16) | (Payload[7] << 8) | Payload[8]);
                                                TemperatureSensors = Payload[9];
                                                TemperatureUnit = Payload[10];
                                                NTCBetaValue = (Payload[11] << 8) | Payload[12];

                                                SettingsGroupBox.BeginInvoke((MethodInvoker)delegate
                                                {
                                                    WaterPumpSpeedTrackBar.Value = WaterPumpSpeed;
                                                    WaterPumpSpeedTrackBar_Scroll(this, EventArgs.Empty);
                                                    WaterPumpOnTimeTextBox.Text = (WaterPumpOnTime / 60000).ToString("0");
                                                    WaterPumpOffTimeTextBox.Text = (WaterPumpOffTime / 60000).ToString("0");
                                                    TemperatureSensorsComboBox.SelectedIndex = TemperatureSensors;

                                                    switch (TemperatureSensors)
                                                    {
                                                        case 0: // None
                                                            TemperaturesGroupBox.Enabled = false;
                                                            TemperatureUnitLabel.Enabled = false;
                                                            TemperatureUnitComboBox.Enabled = false;
                                                            break;
                                                        case 1: // External sensor only
                                                            TemperaturesGroupBox.Enabled = true;
                                                            ExternalTemperatureLabel.Enabled = true;
                                                            ExternalTemperatureTextBox.Enabled = true;
                                                            TemperatureUnitLabel01.Enabled = true;
                                                            InternalTemperatureLabel.Enabled = false;
                                                            InternalTemperatureTextBox.Enabled = false;
                                                            TemperatureUnitLabel02.Enabled = false;
                                                            TemperatureUnitLabel.Enabled = true;
                                                            TemperatureUnitComboBox.Enabled = true;
                                                            break;
                                                        case 2: // Internal sensor only
                                                            TemperaturesGroupBox.Enabled = true;
                                                            ExternalTemperatureLabel.Enabled = false;
                                                            ExternalTemperatureTextBox.Enabled = false;
                                                            TemperatureUnitLabel01.Enabled = false;
                                                            InternalTemperatureLabel.Enabled = true;
                                                            InternalTemperatureTextBox.Enabled = true;
                                                            TemperatureUnitLabel02.Enabled = true;
                                                            TemperatureUnitLabel.Enabled = true;
                                                            TemperatureUnitComboBox.Enabled = true;
                                                            break;
                                                        case 3: // Both external and internal sensors enabled
                                                            TemperaturesGroupBox.Enabled = true;
                                                            ExternalTemperatureLabel.Enabled = true;
                                                            ExternalTemperatureTextBox.Enabled = true;
                                                            TemperatureUnitLabel01.Enabled = true;
                                                            InternalTemperatureLabel.Enabled = true;
                                                            InternalTemperatureTextBox.Enabled = true;
                                                            TemperatureUnitLabel02.Enabled = true;
                                                            TemperatureUnitLabel.Enabled = true;
                                                            TemperatureUnitComboBox.Enabled = true;
                                                            break;
                                                        default:
                                                            TemperaturesGroupBox.Enabled = false;
                                                            TemperatureUnitLabel.Enabled = false;
                                                            TemperatureUnitComboBox.Enabled = false;
                                                            break;
                                                    }

                                                    switch (TemperatureUnit)
                                                    {
                                                        case 1:
                                                            TemperatureUnitComboBox.SelectedIndex = 0; // Celsius
                                                            TemperatureUnitLabel01.Text = "°C";
                                                            TemperatureUnitLabel02.Text = "°C";
                                                            break;
                                                        case 2:
                                                            TemperatureUnitComboBox.SelectedIndex = 1; // Fahrenheit
                                                            TemperatureUnitLabel01.Text = "°F";
                                                            TemperatureUnitLabel02.Text = "°F";
                                                            break;
                                                        case 4:
                                                            TemperatureUnitComboBox.SelectedIndex = 2; // Kelvin
                                                            TemperatureUnitLabel01.Text = "K";
                                                            TemperatureUnitLabel02.Text = "K";
                                                            break;
                                                        default:
                                                            TemperatureUnitComboBox.SelectedIndex = 0; // Celsius
                                                            TemperatureUnitLabel01.Text = "°C";
                                                            TemperatureUnitLabel02.Text = "°C";
                                                            break;
                                                    }

                                                    NTCBetaValueTextBox.Text = NTCBetaValue.ToString("0");

                                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Current settings:" + Environment.NewLine +
                                                                                             "       - water pump speed: " + WaterPumpSpeedTrackBar.Value.ToString("0") + "%" + Environment.NewLine +
                                                                                             "       - water pump on-time: " + WaterPumpOnTimeTextBox.Text + " minutes" + Environment.NewLine +
                                                                                             "       - water pump off-time: " + WaterPumpOffTimeTextBox.Text + " minutes" + Environment.NewLine +
                                                                                             "       - temperature sensors: " + TemperatureSensorsComboBox.Text + Environment.NewLine +
                                                                                             "       - temperature unit: " + TemperatureUnitComboBox.Text + Environment.NewLine +
                                                                                             "       - NTC beta value: " + NTCBetaValueTextBox.Text + " K", null);
                                                });
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            }
                                            break;
                                        case 0x02: // write settings confirmation
                                            if ((Payload.Length > 0) && (Payload[0] == 0x00))
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Settings updated", packet);
                                                Serial.Write(ReadSettings, 0, ReadSettings.Length);
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Settings update error", packet);
                                            }
                                            break;
                                        case 0x03: // auto status update
                                            if (Payload.Length > 0)
                                            {
                                                if (Payload[0] == 0x00)
                                                {
                                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Regular status update response", packet);
                                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Regular status update disabled", null);
                                                }
                                                else
                                                {
                                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Regular status update response", packet);
                                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Regular status update enabled", null);
                                                }
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            }
                                            break;
                                        case 0x04: // service mode
                                            if (Payload.Length > 0)
                                            {
                                                if (Payload[0] == 0x00)
                                                {
                                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Service mode response", packet);
                                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Service mode disabled", null);
                                                }
                                                else
                                                {
                                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Service mode response", packet);
                                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Service mode enabled", null);
                                                }
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            }
                                            break;
                                        case 0x05: // water pump on/off
                                            if (Payload.Length > 0)
                                            {
                                                if (Payload[0] == 0x00)
                                                {
                                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Water pump status", packet);
                                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Water pump turned off", null);
                                                }
                                                else
                                                {
                                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Water pump status", packet);
                                                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Water pump turned on @ " + Math.Round((float)(Payload[0] + 1) / 256 * 100).ToString("0") + "%", null);
                                                }
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            }
                                            break;
                                        default:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            break;
                                    }
                                    break;
                                case 0x05: // response to request
                                    switch (SubDataCode)
                                    {
                                        case 0x01: // EEPROM content
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] EEPROM content", packet);
                                            Util.UpdateTextBox(CommunicationTextBox, "[INFO] First 32 bytes of the EEPROM", Payload);
                                            break;
                                        case 0x02: // hardware/firmware version
                                            if (Payload.Length > 25)
                                            {
                                                string HardwareVersionString = "V" + ((Payload[0] << 8 | Payload[1]) / 100.00).ToString("0.00").Replace(",", ".");
                                                DateTime _HardwareDate = Util.UnixTimeStampToDateTime(Payload[2] << 56 | Payload[3] << 48 | Payload[4] << 40 | Payload[5] << 32 | Payload[6] << 24 | Payload[7] << 16 | Payload[8] << 8 | Payload[9]);
                                                DateTime _AssemblyDate = Util.UnixTimeStampToDateTime(Payload[10] << 56 | Payload[11] << 48 | Payload[12] << 40 | Payload[13] << 32 | Payload[14] << 24 | Payload[15] << 16 | Payload[16] << 8 | Payload[17]);
                                                DateTime _FirmwareDate = Util.UnixTimeStampToDateTime(Payload[18] << 56 | Payload[19] << 48 | Payload[20] << 40 | Payload[21] << 32 | Payload[22] << 24 | Payload[23] << 16 | Payload[24] << 8 | Payload[25]);
                                                string _HardwareDateString = _HardwareDate.ToString("yyyy.MM.dd HH:mm:ss");
                                                string _AssemblyDateString = _AssemblyDate.ToString("yyyy.MM.dd HH:mm:ss");
                                                string _FirmwareDateString = _FirmwareDate.ToString("yyyy.MM.dd HH:mm:ss");
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Hardware/Firmware information response", packet);
                                                Util.UpdateTextBox(CommunicationTextBox, "[INFO] Hardware ver.: " + HardwareVersionString + Environment.NewLine +
                                                                                            "       Hardware date: " + _HardwareDateString + Environment.NewLine +
                                                                                            "       Assembly date: " + _AssemblyDateString + Environment.NewLine +
                                                                                            "       Firmware date: " + _FirmwareDateString, null);
                                                OldFWUNIXTime = (UInt64)(Payload[18] << 56 | Payload[19] << 48 | Payload[20] << 40 | Payload[21] << 32 | Payload[22] << 24 | Payload[23] << 16 | Payload[24] << 8 | Payload[25]);
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            }
                                            break;
                                        case 0x03: // timestamp
                                            if (Payload.Length > 3)
                                            {
                                                TimeSpan ElapsedTime = TimeSpan.FromMilliseconds(Payload[0] << 24 | Payload[1] << 16 | Payload[2] << 8 | Payload[3]);
                                                DateTime Timestamp = DateTime.Today.Add(ElapsedTime);
                                                string TimestampString = Timestamp.ToString("HH:mm:ss.fff");
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Timestamp response", packet);
                                                Util.UpdateTextBox(CommunicationTextBox, "[INFO] Timestamp: " + TimestampString, null);
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            }
                                            break;
                                        case 0x04: // temperatures
                                            if (Payload.Length > 7)
                                            {
                                                ExternalTemperature = BitConverter.ToSingle(new byte[4] { Payload[0], Payload[1], Payload[2], Payload[3] }, 0);
                                                InternalTemperature = BitConverter.ToSingle(new byte[4] { Payload[4], Payload[5], Payload[6], Payload[7] }, 0);
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Temperature response", packet);

                                                TemperaturesGroupBox.BeginInvoke((MethodInvoker)delegate
                                                {
                                                    switch (TemperatureSensors)
                                                    {
                                                        case 0: // none
                                                        ExternalTemperatureTextBox.Text = "---.-";
                                                            InternalTemperatureTextBox.Text = "---.-";
                                                            Util.UpdateTextBox(CommunicationTextBox, "[INFO] Temperatures:" + Environment.NewLine +
                                                                                                     "       - external: ---.-" + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                                                     "       - internal: ---.-" + TemperatureUnitLabel02.Text, null);
                                                            break;
                                                        case 1: // external
                                                        ExternalTemperatureTextBox.Text = ExternalTemperature.ToString("0.0");
                                                            InternalTemperatureTextBox.Text = "---.-";
                                                            Util.UpdateTextBox(CommunicationTextBox, "[INFO] Temperatures:" + Environment.NewLine +
                                                                                                     "       - external: " + ExternalTemperature.ToString("0.0") + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                                                     "       - internal: ---.-" + TemperatureUnitLabel02.Text, null);
                                                            break;
                                                        case 2: // internal
                                                        ExternalTemperatureTextBox.Text = "---.-";
                                                            InternalTemperatureTextBox.Text = InternalTemperature.ToString("0.0");
                                                            Util.UpdateTextBox(CommunicationTextBox, "[INFO] Temperatures:" + Environment.NewLine +
                                                                                                     "       - external: ---.-" + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                                                     "       - internal: " + InternalTemperature.ToString("0.0") + TemperatureUnitLabel02.Text, null);
                                                            break;
                                                        case 3: // external and internal (both)
                                                        ExternalTemperatureTextBox.Text = ExternalTemperature.ToString("0.0");
                                                            InternalTemperatureTextBox.Text = InternalTemperature.ToString("0.0");
                                                            Util.UpdateTextBox(CommunicationTextBox, "[INFO] Temperatures:" + Environment.NewLine +
                                                                                                     "       - external: " + ExternalTemperature.ToString("0.0") + TemperatureUnitLabel01.Text + Environment.NewLine +
                                                                                                     "       - internal: " + InternalTemperature.ToString("0.0") + TemperatureUnitLabel02.Text, null);
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                });
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            }
                                            break;
                                        default:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            break;
                                    }
                                    break;
                                case 0x0E: // Debug
                                    switch (SubDataCode)
                                    {
                                        case 0x01: // read EEPROM
                                            if (Payload.Length > 4)
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] EEPROM dump", packet);
                                                int offset = Payload[0] << 8 | Payload[1];
                                                int count = Payload[2] << 8 | Payload[3];
                                                byte[] values = Payload.Skip(4).Take(Payload.Length - 4).ToArray();
                                                Util.UpdateTextBox(CommunicationTextBox, "[INFO] EEPROM values (offset=" + Convert.ToString(offset, 16).PadLeft(4, '0').PadRight(3, ' ').ToUpper() + ", count=" + Convert.ToString(count, 16).PadLeft(4, '0').PadRight(3, ' ').ToUpper() + ")", values);
                                            }
                                            else
                                            {
                                                Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            }
                                            break;
                                        default:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                            break;
                                    }
                                    break;
                                case 0x0F: // OK/Error
                                    switch (SubDataCode)
                                    {
                                        case 0x00:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] OK", packet);
                                            break;
                                        case 0x01:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: invalid length", packet);
                                            break;
                                        case 0x02:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: invalid command", packet);
                                            break;
                                        case 0x03:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: invalid sub-data code", packet);
                                            break;
                                        case 0x04:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: invalid payload value(s)", packet);
                                            break;
                                        case 0x05:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: invalid checksum", packet);
                                            break;
                                        case 0x06:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: packet timeout occured", packet);
                                            break;
                                        case 0xFD:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: not enough MCU RAM", packet);
                                            break;
                                        case 0xFE:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: internal error", packet);
                                            break;
                                        case 0xFF:
                                            Util.UpdateTextBox(CommunicationTextBox, "[RX->] Error: fatal error", packet);
                                            break;
                                        default:
                                            break;
                                    }
                                    break;
                                default:
                                    Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received", packet);
                                    break;
                            }
                        }
                    }
                    else
                    {
                        Util.UpdateTextBox(CommunicationTextBox, "[RX->] Data received with checksum error", packet);
                    }

                    bufferlist.RemoveRange(0, FullPacketLength);
                }
                else
                {
                    bufferlist.RemoveAt(0); // remove this byte and see what's next
                }
            }
        }

        private void ReadSettingsButton_Click(object sender, EventArgs e)
        {
            Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Settings request", ReadSettings);
            Serial.Write(ReadSettings, 0, ReadSettings.Length);
        }

        private void DefaultSettingsButton_Click(object sender, EventArgs e)
        {
            WaterPumpSpeedTrackBar.Value = 50;
            WaterPumpSpeedTrackBar_Scroll(this, EventArgs.Empty); // trigger event manually to update percentage label
            WaterPumpOnTimeTextBox.Text = "15";
            WaterPumpOffTimeTextBox.Text = "15";
            NTCBetaValueTextBox.Text = "3950";
            TemperatureUnitComboBox.SelectedIndex = 0;
            TemperatureSensorsComboBox.SelectedIndex = 0;

            Util.UpdateTextBox(CommunicationTextBox, "[INFO] Default settings restored." + Environment.NewLine + 
                                                     "       Click the \"Write settings\" button" + Environment.NewLine +
                                                     "       to update the device.", null);
        }

        private void WriteSettingsButton_Click(object sender, EventArgs e)
        {
            // Get values from controls
            if (WaterPumpSpeedTrackBar.Value == 0) WaterPumpSpeed = 0;
            else WaterPumpSpeed = (byte)((WaterPumpSpeedTrackBar.Value * 256 / 100) - 1); // convert percent to PWM value (0-255)

            uint.TryParse(WaterPumpOnTimeTextBox.Text, out WaterPumpOnTime); // 0 if failed
            if (WaterPumpOnTime < 0) WaterPumpOnTime = 15; // minutes default
            if (WaterPumpOnTime > 71582) WaterPumpOnTime = 71582; // maximum value
            WaterPumpOnTime *= 60 * 1000; // convert minutes to milliseconds

            byte[] WaterPumpOnTimeArray = new byte[4];
            WaterPumpOnTimeArray[0] = (byte)((WaterPumpOnTime >> 24) & 0xFF);
            WaterPumpOnTimeArray[1] = (byte)((WaterPumpOnTime >> 16) & 0xFF);
            WaterPumpOnTimeArray[2] = (byte)((WaterPumpOnTime >> 8) & 0xFF);
            WaterPumpOnTimeArray[3] = (byte)(WaterPumpOnTime & 0xFF);

            uint.TryParse(WaterPumpOffTimeTextBox.Text, out WaterPumpOffTime); // 0 if failed
            if (WaterPumpOffTime < 0) WaterPumpOffTime = 15; // minutes default
            if (WaterPumpOffTime > 71582) WaterPumpOffTime = 71582; // maximum value
            WaterPumpOffTime *= 60 * 1000; // convert minutes to milliseconds

            byte[] WaterPumpOffTimeArray = new byte[4];
            WaterPumpOffTimeArray[0] = (byte)((WaterPumpOffTime >> 24) & 0xFF);
            WaterPumpOffTimeArray[1] = (byte)((WaterPumpOffTime >> 16) & 0xFF);
            WaterPumpOffTimeArray[2] = (byte)((WaterPumpOffTime >> 8) & 0xFF);
            WaterPumpOffTimeArray[3] = (byte)(WaterPumpOffTime & 0xFF);

            TemperatureSensors = (byte)TemperatureSensorsComboBox.SelectedIndex;
            if ((TemperatureSensorsComboBox.SelectedIndex < 0) || (TemperatureSensorsComboBox.SelectedIndex > 3)) TemperatureSensors = 0;

            switch (TemperatureUnitComboBox.SelectedIndex)
            {
                case 0: // Celsius
                    TemperatureUnit = 1;
                    break;
                case 1: // Fahrenheit
                    TemperatureUnit = 2;
                    break;
                case 2: // Kelvin
                    TemperatureUnit = 4;
                    break;
                default: // Celsius
                    TemperatureUnit = 1;
                    break;
            }

            int.TryParse(NTCBetaValueTextBox.Text, out NTCBetaValue); // 0 if failed
            if (NTCBetaValue == 0) NTCBetaValue = 3950; // K default

            byte[] NTCBetaValueArray = new byte[2];
            NTCBetaValueArray[0] = (byte)((NTCBetaValue >> 8) & 0xFF);
            NTCBetaValueArray[1] = (byte)(NTCBetaValue & 0xFF);

            byte[] SettingsPayload = new byte[13];
            SettingsPayload[0] = WaterPumpSpeed;
            SettingsPayload[1] = WaterPumpOnTimeArray[0];
            SettingsPayload[2] = WaterPumpOnTimeArray[1];
            SettingsPayload[3] = WaterPumpOnTimeArray[2];
            SettingsPayload[4] = WaterPumpOnTimeArray[3];
            SettingsPayload[5] = WaterPumpOffTimeArray[0];
            SettingsPayload[6] = WaterPumpOffTimeArray[1];
            SettingsPayload[7] = WaterPumpOffTimeArray[2];
            SettingsPayload[8] = WaterPumpOffTimeArray[3];
            SettingsPayload[9] = TemperatureSensors;
            SettingsPayload[10] = TemperatureUnit;
            SettingsPayload[11] = NTCBetaValueArray[0];
            SettingsPayload[12] = NTCBetaValueArray[1];

            byte[] WriteSettings = new byte[19];
            WriteSettings[0] = 0x3D; // sync
            WriteSettings[1] = 0x00; // length low byte
            WriteSettings[2] = 0x0F; // length high byte
            WriteSettings[3] = 0x03; // data code
            WriteSettings[4] = 0x02; // sub-data code
            
            for (int i = 0; i < SettingsPayload.Length; i++)
            {
                WriteSettings[5 + i] = SettingsPayload[i];
            }

            byte Checksum = 0;
            for (int j = 1; j < WriteSettings.Length - 1; j++)
            {
                Checksum += WriteSettings[j];
            }
            WriteSettings[18] = Checksum;

            Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Write settings", WriteSettings);
            Serial.Write(WriteSettings, 0, WriteSettings.Length);
        }

        private void NTCBetaValueModifyCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (NTCBetaValueModifyCheckBox.Checked)
            {
                NTCBetaValueLabel.Enabled = true;
                NTCBetaValueTextBox.Enabled = true;
                NTCBetaKelvinLabel.Enabled = true;
            }
            else
            {
                NTCBetaValueLabel.Enabled = false;
                NTCBetaValueTextBox.Enabled = false;
                NTCBetaKelvinLabel.Enabled = false;
            }
        }

        private void TemperatureSensorsComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (TemperatureSensorsComboBox.SelectedIndex != 0)
            {
                TemperatureUnitComboBox.Enabled = true;
            }
            else
            {
                TemperatureUnitComboBox.Enabled = false;
            }
        }

        private void StatusButton_Click(object sender, EventArgs e)
        {

            Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Status request", StatusRequest);
            Serial.Write(StatusRequest, 0, StatusRequest.Length);

        }

        private void ResetButton_Click(object sender, EventArgs e)
        {
            Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Reset device", ResetDevice);
            Serial.Write(ResetDevice, 0, ResetDevice.Length);
        }

        private void SendButton_Click(object sender, EventArgs e)
        {
            if (DeviceFound)
            {
                if (SendComboBox.Text != String.Empty)
                {
                    byte[] bytes = Util.HexStringToByte(SendComboBox.Text);
                    if ((bytes.Length > 5) && (bytes != null))
                    {
                        Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Data transmitted", bytes);
                        Serial.Write(bytes, 0, bytes.Length);

                        if (!SendComboBox.Items.Contains(SendComboBox.Text)) // only add unique items (no repeat!)
                        {
                            SendComboBox.Items.Add(SendComboBox.Text); // add command to the list so it can be selected later
                        }
                    }
                }
            }
        }

        private void AboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            about = new AboutForm(this)
            {
                StartPosition = FormStartPosition.CenterParent
            };
            about.ShowDialog();
        }

        private void ShowDebugToolsToolStripMenuItem_CheckedChanged(object sender, EventArgs e)
        {
            if (ShowDebugToolsToolStripMenuItem.Checked)
            {
                DebugGroupBox.Visible = true;
                this.Size = new Size(780, 650); // resize form to expanded view
                this.CenterToScreen();
            }
            else
            {
                DebugGroupBox.Visible = false;
                this.Size = new Size(405, 650); // resize form to collapsed view
                this.CenterToScreen();
            }
        }

        private void GetStatusUpdatesCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (Serial.IsOpen)
            {
                if (GetStatusUpdatesCheckBox.Checked)
                {
                    Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Enable regular status updates", GetStatusUpdatesOn);
                    Serial.Write(GetStatusUpdatesOn, 0, GetStatusUpdatesOn.Length);
                }
                else
                {
                    Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Disable regular status updates", GetStatusUpdatesOff);
                    Serial.Write(GetStatusUpdatesOff, 0, GetStatusUpdatesOff.Length);
                }
            }
        }

        private void SendComboBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Return)
            {
                SendButton.PerformClick();
                e.Handled = true;
            }
        }

        private void UpdateFirmwareToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //if (true)
            if ((UpdatePort != String.Empty) && (OldFWUNIXTime != 0) && DeviceFound)
            {
                Util.UpdateTextBox(CommunicationTextBox, "[INFO] Searching for new device firmware" + Environment.NewLine +
                                                         "       This may take a few seconds...", null);

                // Download latest MHTG.ino file from GitHub
                ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
                try
                {
                    Downloader.DownloadFile(SourceFile, @"Tools/MHTG.ino");
                }
                catch
                {
                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Download error", null);
                }

                if (File.Exists(@"Tools/MHTG.ino"))
                {
                    // Get new UNIX time value from the downloaded file
                    string line = String.Empty;
                    bool done = false;
                    using (Stream stream = File.Open(@"Tools/MHTG.ino", FileMode.Open))
                    {
                        using (StreamReader reader = new StreamReader(stream))
                        {
                            while (!done)
                            {
                                line = reader.ReadLine();
                                if (line.Contains("#define FW_DATE"))
                                {
                                    done = true;
                                }
                            }
                        }
                    }

                    string hexline = line.Substring(16, 18);
                    NewFWUNIXTime = Convert.ToUInt64(hexline, 16);

                    DateTime OldFirmwareDate = Util.UnixTimeStampToDateTime(OldFWUNIXTime);
                    DateTime NewFirmwareDate = Util.UnixTimeStampToDateTime(NewFWUNIXTime);
                    string OldFirmwareDateString = OldFirmwareDate.ToString("yyyy.MM.dd HH:mm:ss");
                    string NewFirmwareDateString = NewFirmwareDate.ToString("yyyy.MM.dd HH:mm:ss");

                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] Old firmware date: " + OldFirmwareDateString, null);
                    Util.UpdateTextBox(CommunicationTextBox, "[INFO] New firmware date: " + NewFirmwareDateString, null);

                    if (NewFWUNIXTime > OldFWUNIXTime)
                    {
                        Util.UpdateTextBox(CommunicationTextBox, "[INFO] Downloading new firmware", null);
                        Downloader.DownloadFile(FlashFile, @"Tools/MHTG.ino.standard.hex");
                        Util.UpdateTextBox(CommunicationTextBox, "[INFO] Beginning firmware update", null);
                        ConnectButton.PerformClick(); // disconnect
                        Thread.Sleep(500); // wait until UI updates its controls
                        this.Refresh();
                        Process process = new Process();
                        process.StartInfo.WorkingDirectory = "Tools";
                        process.StartInfo.FileName = "avrdude.exe";
                        process.StartInfo.Arguments = "-C avrdude.conf -p m328p -c arduino -P " + UpdatePort + " -b 115200 -D -U flash:w:MHTG.ino.standard.hex:i";
                        process.Start();
                        process.WaitForExit();
                        this.Refresh();
                        Util.UpdateTextBox(CommunicationTextBox, "[INFO] Device firmware update finished" + Environment.NewLine + "       Connect again manually", null);
                        File.Delete(@"Tools/MHTG.ino");
                        File.Delete(@"Tools/MHTG.ino.standard.hex");
                    }
                    else
                    {
                        Util.UpdateTextBox(CommunicationTextBox, "[INFO] No device firmware update available", null);
                        File.Delete(@"Tools/MHTG.ino");
                    }
                }
            }
            else
            {
                Util.UpdateTextBox(CommunicationTextBox, "[INFO] Device firmware update error", null);
            }
        }

        private void ServiceModeToolStripMenuItem_CheckedChanged(object sender, EventArgs e)
        {
            if (Serial.IsOpen)
            {
                if (ServiceModeToolStripMenuItem.Checked)
                {
                    Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Enable service mode", ServiceModeOn);
                    Serial.Write(ServiceModeOn, 0, ServiceModeOn.Length);
                }
                else
                {
                    Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Disable service mode", ServiceModeOff);
                    Serial.Write(ServiceModeOff, 0, ServiceModeOff.Length);
                }
            }
        }

        private void WaterPumpOnOffCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (Serial.IsOpen)
            {
                if (WaterPumpOnOffCheckBox.Checked)
                {
                    TurnOnWaterPump[5] = (byte)((WaterPumpSpeedTrackBar.Value * 256 / 100) - 1); // payload contains pwm-level

                    byte checksum = 0;
                    for (int i = 1; i < 6; i++)
                    {
                        checksum += TurnOnWaterPump[i];
                    }
                    TurnOnWaterPump[6] = checksum; // re-calculate checksum

                    Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Turn on water pump", TurnOnWaterPump);
                    Serial.Write(TurnOnWaterPump, 0, TurnOnWaterPump.Length);
                }
                else
                {
                    Util.UpdateTextBox(CommunicationTextBox, "[<-TX] Turn off water pump", TurnOffWaterPump);
                    Serial.Write(TurnOffWaterPump, 0, TurnOffWaterPump.Length);
                }
            }
        }
    }
}