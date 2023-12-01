using System;

using wclCommon;
using wclCommunication;
using wclBluetooth;

namespace MultyGatt
{
    internal sealed class GattClient : wclGattClient
    {
        #region Attribute UUIDs
        private static readonly Guid SERVICE_UUID = new Guid("a25fede7-395c-4241-adc0-481004d81900");

        private static readonly Guid NOTIFIABLE_CHARACTERISTIC_UUID = new Guid("eabbbb91-b7e5-4e50-b7aa-ced2be8dfbbe");
        private static readonly Guid READABLE_CHARACTERISTIC_UUID = new Guid("468dfe19-8de3-4181-b728-0902c50a5e6d");
        private static readonly Guid WRITABLE_CHARACTERISTIC_UUID = new Guid("421754b0-e70a-42c9-90ed-4aed82fa7ac0");
        #endregion Attribute UUIDs

        #region Private fields
        private Boolean FConnected;
        private Object FCS;

        #region Attributes
        private wclGattCharacteristic? FReadableChar;
        private wclGattCharacteristic? FWritableChar;
        #endregion Attributes
        #endregion Private fields

        #region GATT Client overrides
        // The method called when connection procedure completed (with or without success).
        // If the Error parameter is WCL_E_SUCCESS then we are connected and can read attributes
        // and subscribe. If something goes wrong during attributes reading and subscribing we
        // have to disconnect from the device.
        // If the Error parameter is not WCL_E_SUCCESS then connection failed. Simple call
        // OnConnect event with error and do nothing more.
        protected override void DoConnect(Int32 Error)
        {
            // If connection was success try to read required attributes.
            if (Error == wclErrors.WCL_E_SUCCESS)
            {
                wclGattUuid Uuid = new wclGattUuid();
                Uuid.IsShortUuid = false;

                // First find required service.
                Uuid.LongUuid = SERVICE_UUID;
                wclGattService? Service;
                Error = FindService(Uuid, out Service);
                if (Error == wclErrors.WCL_E_SUCCESS)
                {
                    // Service found. Try to find readable characteristic.
                    Uuid.LongUuid = READABLE_CHARACTERISTIC_UUID;
                    Error = FindCharacteristic(Service.Value, Uuid, out FReadableChar);
                    if (Error == wclErrors.WCL_E_SUCCESS)
                    {
                        // If readable characteristic found try to find writable one.
                        Uuid.LongUuid = WRITABLE_CHARACTERISTIC_UUID;
                        Error = FindCharacteristic(Service.Value, Uuid, out FWritableChar);
                        if (Error == wclErrors.WCL_E_SUCCESS)
                        {
                            // Ok, we got writable characteristic. Now try to find notifiable one.
                            Uuid.LongUuid = NOTIFIABLE_CHARACTERISTIC_UUID;
                            wclGattCharacteristic? NotifiableChar;
                            Error = FindCharacteristic(Service.Value, Uuid, out NotifiableChar);
                            if (Error == wclErrors.WCL_E_SUCCESS)
                            {
                                // Notifiable characteristic found. Try to subscribe. We do not need to
                                // save the characteristic globally. It will be unsubscribed during disconnection.
                                Error = SubscribeForNotifications(NotifiableChar.Value);

                                // If subscribed - set connected flag.
                                if (Error == wclErrors.WCL_E_SUCCESS)
                                {
                                    lock (FCS)
                                    {
                                        FConnected = true;
                                    }
                                }
                            }

                            // If something went wrong - cleanup writable characteristic.
                            if (Error != wclErrors.WCL_E_SUCCESS)
                                FWritableChar = null;
                        }

                        // If something went wrong - cleanup readable characteristic.
                        if (Error != wclErrors.WCL_E_SUCCESS)
                            FReadableChar = null;
                    }
                }

                // If something went wrong we must disconnect!
                if (Error != wclErrors.WCL_E_SUCCESS)
                    Disconnect();
            }

            // Call inherited method anyway so the OnConnect event fires.
            base.DoConnect(Error);
        }

        // The method called when the remote device disconnected. The Reason parameter
        // indicates disconnection reason.
        protected override void DoDisconnect(Int32 Reason)
        {
            // Make sure that we were "connected".
            if (FConnected)
            {
                // Simple cleanup.
                lock (FCS)
                {
                    FConnected = false;
                }

                FReadableChar = null;
                FWritableChar = null;
            }

            // Call the inherited method to fire the OnDisconnect event.
            base.DoDisconnect(Reason);
        }
        #endregion GATT Client overrides

        #region Constructor and Destructor
        public GattClient()
            : base()
        {
            FConnected = false;
            FCS = new Object();

            FReadableChar = null;
            FWritableChar = null;
        }
        #endregion Constructor and Destructor

        #region Connection and disconnection
        // Override connect method. We need it for thread synchronization.
        public Int32 Connect(Int64 Address, wclBluetoothRadio Radio)
        {
            if (Address == 0 || Radio == null)
                return wclErrors.WCL_E_INVALID_ARGUMENT;

            lock (FCS)
            {
                if (FConnected || State != wclClientState.csDisconnected)
                    return wclConnectionErrors.WCL_E_CONNECTION_ACTIVE;

                this.Address = Address;
                return base.Connect(Radio);
            }
        }

        // Override disconnect method. We need it for thread synchronization.
        public new Int32 Disconnect()
        {
            lock (FCS)
            {
                if (!FConnected)
                    return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE;

                return base.Disconnect();
            }
        }
        #endregion Connection and disconnection

        #region Reading and writing values
        // Simple read value from the readable characteristic.
        public Int32 ReadValue(out Byte[] Value)
        {
            Value = null;

            lock (FCS)
            {
                if (!FConnected)
                    return wclConnectionErrors.WCL_E_CONNECTION_CLOSED;

                // Always use Read From Device.
                return ReadCharacteristicValue(FReadableChar.Value, wclGattOperationFlag.goReadFromDevice,
                    out Value);
            }
        }

        // Simple write value to the writable characteristic.
        public Int32 WriteValue(Byte[] Value)
        {
            if (Value == null || Value.Length == 0)
                return wclErrors.WCL_E_INVALID_ARGUMENT;

            lock (FCS)
            {
                if (!FConnected)
                    return wclConnectionErrors.WCL_E_CONNECTION_CLOSED;

                return WriteCharacteristicValue(FWritableChar.Value, Value);
            }
        }
        #endregion Reading and writing values
    };
}
