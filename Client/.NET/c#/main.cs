using System;
using System.Text;
using System.Windows.Forms;

using wclCommon;
using wclBluetooth;

namespace MultiGatt
{
    public partial class fmMain : Form
    {
        private enum DeviceStatus
        {
            dsFound,
            dsConnecting,
            dsConnected
        };

        private wclBluetoothManager FManager;
        private ClientWatcher FWatcher;

        #region Helper methods
        private void DisableButtons(Boolean Started)
        {
            btStart.Enabled = !Started;
            btStop.Enabled = Started;

            btDisconnect.Enabled = false;
            btSend.Enabled = false;
            btRead.Enabled = false;
        }

        private void UpdateButtons()
        {
            if (lvDevices.SelectedItems.Count > 0)
            {
                ListViewItem Item = lvDevices.SelectedItems[0];
                DeviceStatus Status = (DeviceStatus)Item.Tag;

                btDisconnect.Enabled = (Status == DeviceStatus.dsConnected);
                btRead.Enabled = (Status == DeviceStatus.dsConnected);
                btSend.Enabled = (Status == DeviceStatus.dsConnected && edData.Text != "");
            }
            else
            {
                btDisconnect.Enabled = false;
                btRead.Enabled = false;
                btSend.Enabled = false;
            }
        }
        #endregion Helper methods

        #region Client Watcher event handlers
        private void WatcherStopped(Object sender, EventArgs e)
        {
            if (InvokeRequired)
                BeginInvoke(new EventHandler(WatcherStopped), new[] { sender, e });
            else
            {
                DisableButtons(false);
                lvDevices.Items.Clear();

                lbLog.Items.Add("Client watcher stopped");

                FManager.Close();
            }
        }

        private void WatcherStarted(Object sender, EventArgs e)
        {
            if (InvokeRequired)
                BeginInvoke(new EventHandler(WatcherStopped), new[] { sender, e });
            else
            {
                DisableButtons(true);
                lvDevices.Items.Clear();

                lbLog.Items.Add("Client watcher started");
            }
        }

        private void WatcherValueChanged(Int64 Address, Byte[] Value)
        {
            if (InvokeRequired)
                BeginInvoke(new ClientValueChanged(WatcherValueChanged), new Object[] { Address, Value });
            else
            {
                if (Value != null && Value.Length > 3)
                {
                    UInt32 Val = (UInt32)((Value[3] << 24) | (Value[2] << 16) | (Value[1] << 8) | Value[0]);
                    lbLog.Items.Add("Data received from " + Address.ToString("X12") + ": " + Val.ToString());
                }
                else
                    lbLog.Items.Add("Empty data received");
            }
        }

        private void WatcherDeviceFound(Int64 Address, String Name)
        {
            if (InvokeRequired)
                BeginInvoke(new ClientDeviceFound(WatcherDeviceFound), new Object[] { Address, Name });
            else
            {
                lbLog.Items.Add("Device " + Address.ToString("X12") + " found: " + Name);

                ListViewItem Item = lvDevices.Items.Add(Address.ToString("X12"));
                Item.SubItems.Add(Name);
                Item.SubItems.Add("Found...");
                Item.Tag = DeviceStatus.dsFound;
            }
        }

        private void WatcherConnectionStarted(Int64 Address, Int32 Result)
        {
            if (InvokeRequired)
                BeginInvoke(new ClientConnectionStarted(WatcherConnectionStarted), new Object[] { Address, Result });
            else
            {
                lbLog.Items.Add("Connection to " + Address.ToString("X12") + " started: 0x" + Result.ToString("X8"));

                if (lvDevices.Items.Count > 0)
                {
                    foreach (ListViewItem Item in lvDevices.Items)
                    {
                        Int64 DeviceAddress = Convert.ToInt64(Item.Text, 16);
                        if (DeviceAddress == Address)
                        {
                            if (Result != wclErrors.WCL_E_SUCCESS)
                                lvDevices.Items.Remove(Item);
                            else
                            {
                                Item.SubItems[2].Text = "Connecting...";
                                Item.Tag = DeviceStatus.dsConnecting;
                            }
                            break;
                        }
                    }
                }
            }
        }

        private void WatcherConnectionCompleted(Int64 Address, Int32 Result)
        {
            if (InvokeRequired)
                BeginInvoke(new ClientConnectionCompleted(WatcherConnectionCompleted), new Object[] { Address, Result });
            else
            {
                lbLog.Items.Add("Connection to " + Address.ToString("X12") + " completed: 0x" + Result.ToString("X8"));

                if (lvDevices.Items.Count > 0)
                {
                    foreach (ListViewItem Item in lvDevices.Items)
                    {
                        Int64 DeviceAddress = Convert.ToInt64(Item.Text, 16);
                        if (DeviceAddress == Address)
                        {
                            if (Result != wclErrors.WCL_E_SUCCESS)
                                lvDevices.Items.Remove(Item);
                            else
                            {
                                Item.SubItems[2].Text = "Connected";
                                Item.Tag = DeviceStatus.dsConnected;
                                UpdateButtons();
                            }
                            break;
                        }
                    }
                }
            }
        }

        private void WatcherClientDisconnected(Int64 Address, Int32 Reason)
        {
            if (InvokeRequired)
                BeginInvoke(new ClientDisconnected(WatcherClientDisconnected), new Object[] { Address, Reason });
            else
            {
                lbLog.Items.Add("Device " + Address.ToString("X12") + " disconnected: 0x" + Reason.ToString("X8"));

                if (lvDevices.Items.Count > 0)
                {
                    foreach (ListViewItem Item in lvDevices.Items)
                    {
                        Int64 DeviceAddress = Convert.ToInt64(Item.Text, 16);
                        if (DeviceAddress == Address)
                        {
                            lvDevices.Items.Remove(Item);
                            UpdateButtons();
                            break;
                        }
                    }
                }
            }
        }
        #endregion Client Watcher event handlers

        #region Bluetooth Manager event handlers
        private void ManagerClosed(Object sender, EventArgs e)
        {
            if (InvokeRequired)
                BeginInvoke(new EventHandler(ManagerClosed), new[] { sender, e });
            else
                lbLog.Items.Add("Bluetooth Manager closed");
        }

        private void ManagerBeforeClose(Object sender, EventArgs e)
        {
            if (InvokeRequired)
                BeginInvoke(new EventHandler(ManagerBeforeClose), new[] { sender, e });
            else
                lbLog.Items.Add("Bluetooth Manager is closing");
        }

        private void ManagerAfterOpen(Object sender, EventArgs e)
        {
            if (InvokeRequired)
                BeginInvoke(new EventHandler(ManagerAfterOpen), new[] { sender, e });
            else
                lbLog.Items.Add("Bluetooth Manager opened");
        }
        #endregion Bluetooth Manager event handlers

        #region Form event handlers
        private void fmMain_Load(Object sender, EventArgs e)
        {
            // Do not forget to change synchronization method.
            wclMessageBroadcaster.SetSyncMethod(wclMessageSynchronizationKind.skThread);

            FManager = new wclBluetoothManager();
            FManager.AfterOpen += ManagerAfterOpen;
            FManager.BeforeClose += ManagerBeforeClose;
            FManager.OnClosed += ManagerClosed;

            FWatcher = new ClientWatcher();
            FWatcher.OnClientDisconnected += WatcherClientDisconnected;
            FWatcher.OnConnectionCompleted += WatcherConnectionCompleted;
            FWatcher.OnConnectionStarted += WatcherConnectionStarted;
            FWatcher.OnDeviceFound += WatcherDeviceFound;
            FWatcher.OnValueChanged += WatcherValueChanged;
            FWatcher.OnStarted += WatcherStarted;
            FWatcher.OnStopped += WatcherStopped;
        }

        private void fmMain_FormClosed(Object sender, FormClosedEventArgs e)
        {
            btStop_Click(btStop, EventArgs.Empty);
        }
        #endregion Form event handlers

        #region Buttons event handlers
        private void btStart_Click(Object sender, EventArgs e)
        {
            Int32 Res = FManager.Open();
            if (Res != wclErrors.WCL_E_SUCCESS)
                MessageBox.Show("Open Bluetooth Manager failed: 0x" + Res.ToString("X8"));
            else
            {
                wclBluetoothRadio Radio;
                Res = FManager.GetLeRadio(out Radio);
                if (Res != wclErrors.WCL_E_SUCCESS)
                    MessageBox.Show("Get working radio failed: 0x" + Res.ToString("X8"));
                else
                {
                    Res = FWatcher.Start(Radio);
                    if (Res != wclErrors.WCL_E_SUCCESS)
                        MessageBox.Show("Start Watcher failed: 0x" + Res.ToString("X8"));
                }

                if (Res != wclErrors.WCL_E_SUCCESS)
                    FManager.Close();
            }
        }

        private void btStop_Click(Object sender, EventArgs e)
        {
            FWatcher.Stop();
        }

        private void btDisconnect_Click(Object sender, EventArgs e)
        {
            if (lvDevices.SelectedItems.Count > 0)
            {
                ListViewItem Item = lvDevices.SelectedItems[0];
                Int64 Address = Convert.ToInt64(Item.Text, 16);
                Int32 Res = FWatcher.Disconnect(Address);
                if (Res != wclErrors.WCL_E_SUCCESS)
                    MessageBox.Show("Disconnect failed: 0x" + Res.ToString("X8"));
            }
        }

        private void btRead_Click(Object sender, EventArgs e)
        {
            if (lvDevices.SelectedItems.Count > 0)
            {
                ListViewItem Item = lvDevices.SelectedItems[0];
                Int64 Address = Convert.ToInt64(Item.Text, 16);
                Byte[] Data;
                Int32 Res = FWatcher.ReadData(Address, out Data);
                if (Res != wclErrors.WCL_E_SUCCESS)
                    MessageBox.Show("Read failed: 0x" + Res.ToString("X8"));
                else
                {
                    if (Data == null || Data.Length == 0)
                        MessageBox.Show("Data is empty");
                    else
                    {
                        String s = Encoding.ASCII.GetString(Data);
                        MessageBox.Show("Data read: " + s);
                    }
                }
            }
        }

        private void btSend_Click(Object sender, EventArgs e)
        {
            if (lvDevices.SelectedItems.Count > 0)
            {
                ListViewItem Item = lvDevices.SelectedItems[0];
                Int64 Address = Convert.ToInt64(Item.Text, 16);
                Byte[] Data = Encoding.ASCII.GetBytes(edData.Text);
                Int32 Res = FWatcher.WriteData(Address, Data);
                if (Res != wclErrors.WCL_E_SUCCESS)
                    MessageBox.Show("Write failed: 0x" + Res.ToString("X8"));
            }
        }

        private void btClear_Click(Object sender, EventArgs e)
        {
            lbLog.Items.Clear();
        }
        #endregion Buttons event handlers

        #region ListView event handlers
        private void lvDevices_SelectedIndexChanged(Object sender, EventArgs e)
        {
            UpdateButtons();
        }
        #endregion ListView event handlers

        #region TextBox event handlers
        private void edData_TextChanged(Object sender, EventArgs e)
        {
            UpdateButtons();
        }
        #endregion TextBox event handlers

        public fmMain()
        {
            InitializeComponent();
        }
    }
}
