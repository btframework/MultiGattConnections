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
    Private FConnections As List(Of GattClient)
    Private FPendingConnections As List(Of Int64)
#End Region

#Region "Helper method"
    Private Sub RemoveClient(Client As GattClient)
        If Client Is Nothing Then Return

        SyncLock (FConnectionsCS)
            ' Remove client from the connections list.
            If FPendingConnections.Contains(Client.Address) Then
                RemoveHandler Client.OnCharacteristicChanged, AddressOf ClientCharacteristicChanged
                RemoveHandler Client.OnConnect, AddressOf ClientConnect
                RemoveHandler Client.OnDisconnect, AddressOf ClientDisconnect
                FPendingConnections.Remove(Client.Address)
            End If

            ' Remove client from the clients list.
            If FConnections.Contains(Client) Then FConnections.Remove(Client)
        End SyncLock
    End Sub

    Private Function FindClient(Address As Int64) As GattClient
        If FConnections.Count = 0 Then Return Nothing

        For Each Client As GattClient In FConnections
            If Client.Address = Address Then Return Client
        Next

        Return Nothing
    End Function
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
            ' Disconnect client And set the connection error.
            If [Error] = wclErrors.WCL_E_SUCCESS Then
                Client.Disconnect()
            Else
                RemoveClient(Client)
            End If
            Return
        End If

        ' If connection failed remove client from connections list.
        If [Error] <> wclErrors.WCL_E_SUCCESS Then
            RemoveClient(Client)
        Else
            ' Othewrwise - add it to the connected clients list.
            SyncLock (FConnectionsCS)
                FConnections.Add(Client)
            End SyncLock
        End If

        ' Call connection completed event.
        DoConnectionCompleted(Client.Address, [Error])
    End Sub

    Private Sub ClientCharacteristicChanged(Sender As Object, Handle As UInt16, Value As Byte())
        If Monitoring Then
            Dim Client As GattClient = CType(Sender, GattClient)
            ' Simple call the value changed event.
            DoValueChanged(Client.Address, Value)
        End If
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

        SyncLock (FConnectionsCS)
            ' Make sure that device Is Not in connections list.
            If FPendingConnections.Contains(Address) Then Return

            ' Check devices name.
            If Name = DEVICE_NAME Then
                ' Notify about New device.
                DoDeviceFound(Address, Name)

                ' Create client.
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
                    FPendingConnections.Add(Address)
                End If
            End If
        End SyncLock
    End Sub

    Protected Overrides Sub DoStopped()
        Dim Clients As List(Of GattClient) = New List(Of GattClient)()
        SyncLock (FConnectionsCS)
            If FConnections.Count > 0 Then
                ' Make copy of the connected clients.
                For Each Client As GattClient In FConnections
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
        FConnections = New List(Of GattClient)()
        FPendingConnections = New List(Of Int64)()

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

        Dim Client As GattClient
        SyncLock (FConnectionsCS)
            Client = FindClient(Address)
            If Client Is Nothing Then Return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE
        End SyncLock
        Return Client.Disconnect()
    End Function

    Public Function ReadData(Address As Int64, ByRef Data As Byte()) As Int32
        Data = Nothing

        If Not Monitoring Then Return wclConnectionErrors.WCL_E_CONNECTION_CLOSED

        SyncLock (FConnectionsCS)
            Dim Client As GattClient = FindClient(Address)
            If Client Is Nothing Then Return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE
            Return Client.ReadValue(Data)
        End SyncLock
    End Function

    Public Function WriteData(Address As Int64, Data As Byte()) As Int32
        If Not Monitoring Then Return wclConnectionErrors.WCL_E_CONNECTION_CLOSED
        If Data Is Nothing OrElse Data.Length = 0 Then Return wclErrors.WCL_E_INVALID_ARGUMENT

        SyncLock (FConnectionsCS)
            Dim Client As GattClient = FindClient(Address)
            If Client Is Nothing Then Return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE
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
