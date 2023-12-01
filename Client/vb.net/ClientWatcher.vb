Friend Delegate Sub ClientConnectionCompleted(Address As Int64, Result As Int32)
Friend Delegate Sub ClientConnectionStarted(Address As Int64, Result As Int32)
Friend Delegate Sub ClientDeviceFound(Address As Int64, Name As String)
Friend Delegate Sub ClientDisconnected(Address As Int64, Reason As Int32)
Friend Delegate Sub ClientValueChanged(Address As Int64, Value As Byte())

Friend NotInheritable Class ClientWatcher
    Inherits wclBluetoothLeBeaconWatcher

    Private Const DEVICE_NAME As String = "MultyGattServer"

#Region "Connections management"
    Private FConnectionsCS As Object
    Private FClients As List(Of GattClient) ' Connected clients.
    Private FConnections As Dictionary(Of Int64, GattClient) ' Pending connections.
    Private FFoundDevices As List(Of Int64)
#End Region

#Region "Helper method"
    Private Sub RemoveClient(Client As GattClient)
        SyncLock FConnectionsCS
            ' Remove client from the connections list.
            If FConnections.ContainsKey(Client.Address) Then
                RemoveHandler Client.OnCharacteristicChanged, AddressOf ClientCharacteristicChanged
                RemoveHandler Client.OnConnect, AddressOf ClientConnect
                RemoveHandler Client.OnDisconnect, AddressOf ClientDisconnect
                FConnections.Remove(Client.Address)
            End If

            ' Remove client from the clients list.
            If FClients.Contains(Client) Then FClients.Remove(Client)
        End SyncLock
    End Sub
#End Region

#Region "Client Event handlers"
    Private Sub ClientDisconnect(Sender As Object, Reason As Int32)
        Dim Client As GattClient = CType(Sender, GattClient)
        ' Remove client from the lists.
        RemoveClient(Client)
        ' Call disconnect event.
        DoClientDisconnected(Client.Address, Reason)
    End Sub

    Private Sub ClientConnect(Sender As Object, [Error] As Int32)
        Dim Client As GattClient = CType(Sender, GattClient)

        ' If we stopped we still can get client connection event.
        If Not Monitoring Then
            ' Remove client from connections list.
            RemoveClient(Client)
            ' Disconnect client And set the connection error.
            Client.Disconnect()
            ' Set the connection error code here.
            [Error] = wclBluetoothErrors.WCL_E_BLUETOOTH_LE_CONNECTION_TERMINATED
        End If

        ' If connection failed remove client from connections list.
        If [Error] <> wclErrors.WCL_E_SUCCESS Then
            ' If it was already remove - nothing happens.
            RemoveClient(Client)
        Else
            ' Othewrwise - add it to the connected clients list.
            SyncLock FConnectionsCS
                FClients.Add(Client)
            End SyncLock
        End If

        ' Call connection completed event.
        DoConnectionCompleted(Client.Address, [Error])
    End Sub

    Private Sub ClientCharacteristicChanged(Sender As Object, Handle As UInt16, Value As Byte())
        Dim Client As GattClient = CType(Sender, GattClient)
        ' Simple call the value changed event.
        DoValueChanged(Client.Address, Value)
    End Sub
#End Region

#Region "Events management"
    Private Sub DoClientDisconnected(Address As Int64, Reason As Int32)
        If OnClientDisconnectedEvent IsNot Nothing Then RaiseEvent OnClientDisconnected(Address, Reason)
    End Sub

    Private Sub DoConnectionCompleted(Address As Int64, Result As Int32)
        If OnConnectionCompletedEvent IsNot Nothing Then RaiseEvent OnConnectionCompleted(Address, Result)
    End Sub

    Private Sub DoConnectionStarted(Address As Int64, Result As Int32)
        If OnConnectionStartedEvent IsNot Nothing Then RaiseEvent OnConnectionStarted(Address, Result)
    End Sub

    Private Sub DoDeviceFound(Address As Int64, Name As String)
        If OnDeviceFoundEvent IsNot Nothing Then RaiseEvent OnDeviceFound(Address, Name)
    End Sub

    Private Sub DoValueChanged(Address As Int64, Value As Byte())
        If OnValueChangedEvent IsNot Nothing Then RaiseEvent OnValueChanged(Address, Value)
    End Sub
#End Region

#Region "Device search handling"
    Protected Overrides Sub DoAdvertisementFrameInformation(Address As Int64, Timestamp As Int64, Rssi As SByte, Name As String, PacketType As wclBluetoothLeAdvertisementType, Flags As wclBluetoothLeAdvertisementFlag)
        ' Do nothing if we stopped.
        If Not Monitoring Then Return

        SyncLock FConnectionsCS
            ' Make sure that device Is Not in connections list.
            If Not FConnections.ContainsKey(Address) Then
                ' Make sure that we did Not see this device early.
                If Not FFoundDevices.Contains(Address) Then
                    ' Check devices name.
                    If Name = DEVICE_NAME Then
                        ' Add device into found list.
                        FFoundDevices.Add(Address)
                        ' Call device found event.
                        DoDeviceFound(Address, Name)
                    End If
                Else
                    ' Device Is in our devices list. Make sure we received connectable advertisement.
                    If PacketType = wclBluetoothLeAdvertisementType.atConnectableDirected OrElse PacketType = wclBluetoothLeAdvertisementType.atConnectableUndirected Then
                        ' We are ready to connect to the device.
                        Dim Client As GattClient = New GattClient()
                        ' Set required event handlers.
                        AddHandler Client.OnCharacteristicChanged, AddressOf ClientCharacteristicChanged
                        AddHandler Client.OnConnect, AddressOf ClientConnect
                        AddHandler Client.OnDisconnect, AddressOf ClientDisconnect
                        ' Try to start connection to the device.
                        Dim Result As Int32 = Client.Connect(Address, Radio)
                        ' Report connection start event.
                        DoConnectionStarted(Address, Result)
                        ' If connection started with success...
                        If Result = wclErrors.WCL_E_SUCCESS Then
                            ' ...add device to pending connections list.
                            FConnections.Add(Address, Client)
                        End If

                        ' Now we can remove the device from found devices list.
                        FFoundDevices.Remove(Address)
                    End If
                End If
            End If
        End SyncLock

        MyBase.DoAdvertisementFrameInformation(Address, Timestamp, Rssi, Name, PacketType, Flags)
    End Sub

    Protected Overrides Sub DoStarted()
        ' Clear all lists.
        FClients.Clear()
        FConnections.Clear()
        FFoundDevices.Clear()

        MyBase.DoStarted()
    End Sub

    Protected Overrides Sub DoStopped()
        Dim Clients As List(Of GattClient) = New List(Of GattClient)
        SyncLock FConnectionsCS
            If FClients.Count > 0 Then
                ' Make copy of the connected clients.
                For Each Client As GattClient In FClients
                    Clients.Add(Client)
                Next
            End If
        End SyncLock

        ' Disconnect all connected clients.
        If Clients.Count > 0 Then
            For Each Client As GattClient In Clients
                Client.Disconnect()
            Next
        End If

        MyBase.DoStopped()
    End Sub
#End Region

#Region "Constructor And destructor"
    Sub New()
        MyBase.New()

        FConnectionsCS = New Object()
        FClients = New List(Of GattClient)
        FConnections = New Dictionary(Of Int64, GattClient)
        FFoundDevices = New List(Of Int64)

        OnClientDisconnectedEvent = Nothing
        OnConnectionCompletedEvent = Nothing
        OnConnectionStartedEvent = Nothing
        OnDeviceFoundEvent = Nothing
        OnValueChangedEvent = Nothing
    End Sub
#End Region

#Region "Communication methods"
    Public Function Disconnect(Address As Int64) As Int32
        If Not Monitoring Then Return wclConnectionErrors.WCL_E_CONNECTION_CLOSED

        Dim Client As GattClient = Nothing
        SyncLock FConnectionsCS
            If FConnections.ContainsKey(Address) Then Client = FConnections(Address)
            If Client IsNot Nothing Then
                If Not FClients.Contains(Client) Then Client = Nothing
            End If
        End SyncLock

        If Client Is Nothing Then Return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE
        Return Client.Disconnect()
    End Function

    Public Function ReadData(Address As Int64, ByRef Data As Byte()) As Int32
        Data = Nothing

        If Not Monitoring Then Return wclConnectionErrors.WCL_E_CONNECTION_CLOSED

        SyncLock FConnectionsCS
            If Not FConnections.ContainsKey(Address) Then Return wclBluetoothErrors.WCL_E_BLUETOOTH_LE_DEVICE_NOT_FOUND

            Dim Client As GattClient = FConnections(Address)
            If Not FClients.Contains(Client) Then Return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE

            Return Client.ReadValue(Data)
        End SyncLock
    End Function

    Public Function WriteData(Address As Int64, Data As Byte()) As Int32
        If Not Monitoring Then Return wclConnectionErrors.WCL_E_CONNECTION_CLOSED
        If Data Is Nothing OrElse Data.Length = 0 Then Return wclErrors.WCL_E_INVALID_ARGUMENT

        SyncLock FConnectionsCS
            If Not FConnections.ContainsKey(Address) Then Return wclBluetoothErrors.WCL_E_BLUETOOTH_LE_DEVICE_NOT_FOUND

            Dim Client As GattClient = FConnections(Address)
            If Not FClients.Contains(Client) Then Return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE

            Return Client.WriteValue(Data)
        End SyncLock
    End Function
#End Region

#Region "Events"
    Public Event OnClientDisconnected As ClientDisconnected
    Public Event OnConnectionCompleted As ClientConnectionCompleted
    Public Event OnConnectionStarted As ClientConnectionStarted
    Public Event OnDeviceFound As ClientDeviceFound
    Public Event OnValueChanged As ClientValueChanged
#End Region
End Class
