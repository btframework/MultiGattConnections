Public Class fmMain
    Private Enum DeviceStatus
        dsFound
        dsConnecting
        dsConnected
    End Enum

    Private WithEvents FManager As wclBluetoothManager
    Private WithEvents FWatcher As ClientWatcher

#Region "Helper methods"
    Private Sub DisableButtons(Started As Boolean)
        btStart.Enabled = Not Started
        btStop.Enabled = Started

        btDisconnect.Enabled = False
        btSend.Enabled = False
        btRead.Enabled = False
    End Sub

    Private Sub UpdateButtons()
        If lvDevices.SelectedItems.Count > 0 Then
            Dim Item As ListViewItem = lvDevices.SelectedItems(0)
            Dim Status As DeviceStatus = CType(Item.Tag, DeviceStatus)

            btDisconnect.Enabled = Status = DeviceStatus.dsConnected
            btRead.Enabled = Status = DeviceStatus.dsConnected
            btSend.Enabled = Status = DeviceStatus.dsConnected AndAlso edData.Text <> ""
        Else
            btDisconnect.Enabled = False
            btRead.Enabled = False
            btSend.Enabled = False
        End If
    End Sub
#End Region

#Region "Client Watcher event handlers"
    Private Sub WatcherStopped(sender As Object, e As EventArgs) Handles FWatcher.OnStopped
        If InvokeRequired Then
            BeginInvoke(New EventHandler(AddressOf WatcherStopped), sender, e)
        Else
            DisableButtons(False)
            lvDevices.Items.Clear()

            lbLog.Items.Add("Client watcher stopped")

            FManager.Close()
        End If
    End Sub

    Private Sub WatcherStarted(sender As Object, e As EventArgs) Handles FWatcher.OnStarted
        If InvokeRequired Then
            BeginInvoke(New EventHandler(AddressOf WatcherStopped), sender, e)
        Else
            DisableButtons(True)
            lvDevices.Items.Clear()

            lbLog.Items.Add("Client watcher started")
        End If
    End Sub

    Private Sub WatcherValueChanged(Address As Long, Value() As Byte) Handles FWatcher.OnValueChanged
        If InvokeRequired Then
            BeginInvoke(New ClientValueChanged(AddressOf WatcherValueChanged), Address, Value)
        Else
            If Value IsNot Nothing AndAlso Value.Length > 3 Then
                Dim Val As UInt32 = (Value(3) << 24) Or (Value(2) << 16) Or (Value(1) << 8) Or Value(0)
                lbLog.Items.Add("Data received from " + Address.ToString("X12") + ": " + Val.ToString())
            Else
                lbLog.Items.Add("Empty data received")
            End If
        End If
    End Sub

    Private Sub WatcherDeviceFound(Address As Long, Name As String) Handles FWatcher.OnDeviceFound
        If InvokeRequired Then
            BeginInvoke(New ClientDeviceFound(AddressOf WatcherDeviceFound), Address, Name)
        Else
            lbLog.Items.Add("Device " + Address.ToString("X12") + " found: " + Name)

            Dim Item As ListViewItem = lvDevices.Items.Add(Address.ToString("X12"))
            Item.SubItems.Add(Name)
            Item.SubItems.Add("Found...")
            Item.Tag = DeviceStatus.dsFound
        End If
    End Sub

    Private Sub WatcherConnectionStarted(Address As Long, Result As Integer) Handles FWatcher.OnConnectionStarted
        If InvokeRequired Then
            BeginInvoke(New ClientConnectionStarted(AddressOf WatcherConnectionStarted), Address, Result)
        Else
            lbLog.Items.Add("Connection to " + Address.ToString("X12") + " started: 0x" + Result.ToString("X8"))

            If lvDevices.Items.Count > 0 Then
                For Each Item As ListViewItem In lvDevices.Items
                    Dim DeviceAddress As Int64 = Convert.ToInt64(Item.Text, 16)
                    If DeviceAddress = Address Then
                        If Result <> wclErrors.WCL_E_SUCCESS Then
                            lvDevices.Items.Remove(Item)
                        Else
                            Item.SubItems(2).Text = "Connecting..."
                            Item.Tag = DeviceStatus.dsConnecting
                        End If
                        Exit For
                    End If
                Next
            End If
        End If
    End Sub

    Private Sub WatcherConnectionCompleted(Address As Long, Result As Integer) Handles FWatcher.OnConnectionCompleted
        If InvokeRequired Then
            BeginInvoke(New ClientConnectionCompleted(AddressOf WatcherConnectionCompleted), Address, Result)
        Else
            lbLog.Items.Add("Connection to " + Address.ToString("X12") + " completed: 0x" + Result.ToString("X8"))

            If lvDevices.Items.Count > 0 Then
                For Each Item As ListViewItem In lvDevices.Items
                    Dim DeviceAddress As Int64 = Convert.ToInt64(Item.Text, 16)
                    If DeviceAddress = Address Then
                        If Result <> wclErrors.WCL_E_SUCCESS Then
                            lvDevices.Items.Remove(Item)
                        Else
                            Item.SubItems(2).Text = "Connected"
                            Item.Tag = DeviceStatus.dsConnected
                            UpdateButtons()
                        End If
                        Exit For
                    End If
                Next
            End If
        End If
    End Sub

    Private Sub WatcherClientDisconnected(Address As Long, Reason As Integer) Handles FWatcher.OnClientDisconnected
        If InvokeRequired Then
            BeginInvoke(New ClientDisconnected(AddressOf WatcherClientDisconnected), Address, Reason)
        Else
            lbLog.Items.Add("Device " + Address.ToString("X12") + " disconnected: 0x" + Reason.ToString("X8"))

            If lvDevices.Items.Count > 0 Then
                For Each Item As ListViewItem In lvDevices.Items
                    Dim DeviceAddress As Int64 = Convert.ToInt64(Item.Text, 16)
                    If DeviceAddress = Address Then
                        lvDevices.Items.Remove(Item)
                        UpdateButtons()
                        Exit For
                    End If
                Next
            End If
        End If
    End Sub
#End Region

#Region "Bluetooth Manager event handlers"
    Private Sub ManagerClosed(sender As Object, e As EventArgs) Handles FManager.OnClosed
        If InvokeRequired Then
            BeginInvoke(New EventHandler(AddressOf ManagerClosed), sender, e)
        Else
            lbLog.Items.Add("Bluetooth Manager closed")
        End If
    End Sub

    Private Sub ManagerBeforeClose(sender As Object, e As EventArgs) Handles FManager.BeforeClose
        If InvokeRequired Then
            BeginInvoke(New EventHandler(AddressOf ManagerBeforeClose), sender, e)
        Else
            lbLog.Items.Add("Bluetooth Manager is closing")
        End If
    End Sub

    Private Sub ManagerAfterOpen(sender As Object, e As EventArgs) Handles FManager.AfterOpen
        If InvokeRequired Then
            BeginInvoke(New EventHandler(AddressOf ManagerAfterOpen), sender, e)
        Else
            lbLog.Items.Add("Bluetooth Manager opened")
        End If
    End Sub
#End Region

#Region "Form Event handlers"
    Private Sub fmMain_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        ' Do Not forget to change synchronization method.
        wclMessageBroadcaster.SetSyncMethod(wclMessageSynchronizationKind.skThread)

        FManager = New wclBluetoothManager()
        FWatcher = New ClientWatcher()
    End Sub

    Private Sub fmMain_FormClosed(sender As Object, e As FormClosedEventArgs) Handles MyBase.FormClosed
        btStop_Click(btStop, EventArgs.Empty)
    End Sub
#End Region

#Region "Buttons event handlers"
    Private Sub btStart_Click(sender As Object, e As EventArgs) Handles btStart.Click
        Dim Res As Int32 = FManager.Open()
        If Res <> wclErrors.WCL_E_SUCCESS Then
            MessageBox.Show("Open Bluetooth Manager failed: 0x" + Res.ToString("X8"))
        Else
            Dim Radio As wclBluetoothRadio = Nothing
            Res = FManager.GetLeRadio(Radio)
            If Res <> wclErrors.WCL_E_SUCCESS Then
                MessageBox.Show("Get working radio failed: 0x" + Res.ToString("X8"))
            Else
                Res = FWatcher.Start(Radio)
                If Res <> wclErrors.WCL_E_SUCCESS Then MessageBox.Show("Start Watcher failed: 0x" + Res.ToString("X8"))
            End If

            If Res <> wclErrors.WCL_E_SUCCESS Then FManager.Close()
        End If
    End Sub

    Private Sub btStop_Click(sender As Object, e As EventArgs) Handles btStop.Click
        FWatcher.Stop()
    End Sub

    Private Sub btDisconnect_Click(sender As Object, e As EventArgs) Handles btDisconnect.Click
        If lvDevices.SelectedItems.Count > 0 Then
            Dim Item As ListViewItem = lvDevices.SelectedItems(0)
            Dim Address As Int64 = Convert.ToInt64(Item.Text, 16)
            Dim Res As Int32 = FWatcher.Disconnect(Address)
            If Res <> wclErrors.WCL_E_SUCCESS Then MessageBox.Show("Disconnect failed: 0x" + Res.ToString("X8"))
        End If
    End Sub

    Private Sub btRead_Click(sender As Object, e As EventArgs) Handles btRead.Click
        If lvDevices.SelectedItems.Count > 0 Then
            Dim Item As ListViewItem = lvDevices.SelectedItems(0)
            Dim Address As Int64 = Convert.ToInt64(Item.Text, 16)
            Dim Data As Byte() = Nothing
            Dim Res As Int32 = FWatcher.ReadData(Address, Data)
            If Res <> wclErrors.WCL_E_SUCCESS Then
                MessageBox.Show("Read failed: 0x" + Res.ToString("X8"))
            Else
                If Data Is Nothing OrElse Data.Length = 0 Then
                    MessageBox.Show("Data is empty")
                Else
                    Dim s As String = System.Text.Encoding.ASCII.GetString(Data)
                    MessageBox.Show("Data read: " + s)
                End If
            End If
        End If
    End Sub

    Private Sub btSend_Click(sender As Object, e As EventArgs) Handles btSend.Click
        If lvDevices.SelectedItems.Count > 0 Then
            Dim Item As ListViewItem = lvDevices.SelectedItems(0)
            Dim Address As Int64 = Convert.ToInt64(Item.Text, 16)
            Dim Data As Byte() = System.Text.Encoding.ASCII.GetBytes(edData.Text)
            Dim Res As Int32 = FWatcher.WriteData(Address, Data)
            If Res <> wclErrors.WCL_E_SUCCESS Then MessageBox.Show("Write failed: 0x" + Res.ToString("X8"))
        End If
    End Sub

    Private Sub btClear_Click(sender As Object, e As EventArgs) Handles btClear.Click
        lbLog.Items.Clear()
    End Sub
#End Region

#Region "ListView event handlers"
    Private Sub lvDevices_SelectedIndexChanged(sender As Object, e As EventArgs) Handles lvDevices.SelectedIndexChanged
        UpdateButtons()
    End Sub
#End Region

#Region "TextBox event handlers"
    Private Sub edData_TextChanged(sender As Object, e As EventArgs) Handles edData.TextChanged
        UpdateButtons()
    End Sub
#End Region
End Class
