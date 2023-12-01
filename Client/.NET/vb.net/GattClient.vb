Friend NotInheritable Class GattClient
    Inherits wclGattClient

#Region "Attribute UUIDs"
    Private Shared ReadOnly SERVICE_UUID As Guid = New Guid("a25fede7-395c-4241-adc0-481004d81900")

    Private Shared ReadOnly NOTIFIABLE_CHARACTERISTIC_UUID As Guid = New Guid("eabbbb91-b7e5-4e50-b7aa-ced2be8dfbbe")
    Private Shared ReadOnly READABLE_CHARACTERISTIC_UUID As Guid = New Guid("468dfe19-8de3-4181-b728-0902c50a5e6d")
    Private Shared ReadOnly WRITABLE_CHARACTERISTIC_UUID As Guid = New Guid("421754b0-e70a-42c9-90ed-4aed82fa7ac0")
#End Region

#Region "Private fields"
    Private FConnected As Boolean
    Private FCS As Object

#Region "Attributes"
    Private FReadableChar As wclGattCharacteristic?
    Private FWritableChar As wclGattCharacteristic?
#End Region
#End Region

#Region "GATT Client overrides"
    ' The method called when connection procedure completed (with Or without success).
    ' If the Error parameter Is WCL_E_SUCCESS then we are connected And can read attributes
    ' And subscribe. If something goes wrong during attributes reading And subscribing we
    ' have to disconnect from the device.
    ' If the Error parameter Is Not WCL_E_SUCCESS then connection failed. Simple call
    ' OnConnect event with error And do nothing more.
    Protected Overrides Sub DoConnect([Error] As Int32)
        ' If connection was success try to read required attributes.
        If [Error] = wclErrors.WCL_E_SUCCESS Then
            Dim Uuid As wclGattUuid = New wclGattUuid()
            Uuid.IsShortUuid = False

            ' First find required service.
            Uuid.LongUuid = SERVICE_UUID
            Dim Service As wclGattService? = Nothing
            [Error] = FindService(Uuid, Service)
            If [Error] = wclErrors.WCL_E_SUCCESS Then
                ' Service found. Try to find readable characteristic.
                Uuid.LongUuid = READABLE_CHARACTERISTIC_UUID
                [Error] = FindCharacteristic(Service.Value, Uuid, FReadableChar)
                If [Error] = wclErrors.WCL_E_SUCCESS Then
                    ' If readable characteristic found try to find writable one.
                    Uuid.LongUuid = WRITABLE_CHARACTERISTIC_UUID
                    [Error] = FindCharacteristic(Service.Value, Uuid, FWritableChar)
                    If [Error] = wclErrors.WCL_E_SUCCESS Then
                        ' Ok, we got writable characteristic. Now try to find notifiable one.
                        Uuid.LongUuid = NOTIFIABLE_CHARACTERISTIC_UUID
                        Dim NotifiableChar As wclGattCharacteristic? = Nothing
                        [Error] = FindCharacteristic(Service.Value, Uuid, NotifiableChar)
                        If [Error] = wclErrors.WCL_E_SUCCESS Then
                            ' Notifiable characteristic found. Try to subscribe. We do Not need to
                            ' save the characteristic globally. It will be unsubscribed during disconnection.
                            [Error] = SubscribeForNotifications(NotifiableChar.Value)

                            ' If subscribed - set connected flag.
                            If [Error] = wclErrors.WCL_E_SUCCESS Then
                                SyncLock FCS
                                    FConnected = True
                                End SyncLock
                            End If
                        End If

                        ' If something went wrong - cleanup writable characteristic.
                        If [Error] <> wclErrors.WCL_E_SUCCESS Then FWritableChar = Nothing
                    End If

                    ' If something went wrong - cleanup readable characteristic.
                    If [Error] <> wclErrors.WCL_E_SUCCESS Then FReadableChar = Nothing
                End If
            End If

            ' If something went wrong we must disconnect!
            If [Error] <> wclErrors.WCL_E_SUCCESS Then Disconnect()
        End If

        ' Call inherited method anyway so the OnConnect event fires.
        MyBase.DoConnect([Error])
    End Sub

    ' The method called when the remote device disconnected. The Reason parameter
    ' indicates disconnection reason.
    Protected Overrides Sub DoDisconnect(Reason As Int32)
        ' Make sure that we were "connected".
        If FConnected Then
            ' Simple cleanup.
            SyncLock FCS
                FConnected = False
            End SyncLock

            FReadableChar = Nothing
            FWritableChar = Nothing
        End If

        ' Call the inherited method to fire the OnDisconnect event.
        MyBase.DoDisconnect(Reason)
    End Sub
#End Region

#Region "Constructor And Destructor"
    Sub New()
        MyBase.New()
        FConnected = False
        FCS = New Object()

        FReadableChar = Nothing
        FWritableChar = Nothing
    End Sub
#End Region

#Region "Connection And disconnection"
    ' Override connect method. We need it for thread synchronization.
    Public Overloads Function Connect(Address As Int64, Radio As wclBluetoothRadio) As Int32
        If Address = 0 OrElse Radio Is Nothing Then Return wclErrors.WCL_E_INVALID_ARGUMENT

        SyncLock FCS
            If FConnected OrElse State <> wclClientState.csDisconnected Then Return wclConnectionErrors.WCL_E_CONNECTION_ACTIVE

            Me.Address = Address
            Return MyBase.Connect(Radio)
        End SyncLock
    End Function

    ' Override disconnect method. We need it for thread synchronization.
    Public Overloads Function Disconnect() As Int32
        SyncLock FCS
            If Not FConnected Then Return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE

            Return MyBase.Disconnect()
        End SyncLock
    End Function
#End Region

#Region "Reading And writing values"
    ' Simple read value from the readable characteristic.
    Public Function ReadValue(ByRef Value As Byte()) As Int32
        Value = Nothing

        SyncLock FCS
            If Not FConnected Then Return wclConnectionErrors.WCL_E_CONNECTION_CLOSED

            ' Always use Read From Device.
            Return ReadCharacteristicValue(FReadableChar.Value, wclGattOperationFlag.goReadFromDevice, Value)
        End SyncLock
    End Function

    ' Simple write value to the writable characteristic.
    Public Function WriteValue(Value As Byte()) As Int32
        If Value Is Nothing OrElse Value.Length = 0 Then Return wclErrors.WCL_E_INVALID_ARGUMENT

        SyncLock FCS
            If Not FConnected Then Return wclConnectionErrors.WCL_E_CONNECTION_CLOSED

            Return WriteCharacteristicValue(FWritableChar.Value, Value)
        End SyncLock
    End Function
#End Region
End Class
