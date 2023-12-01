using System;
using System.Collections.Generic;

using wclCommon;
using wclCommunication;
using wclBluetooth;

namespace MultiGatt
{
    internal delegate void ClientConnectionCompleted(Int64 Address, Int32 Result);
    internal delegate void ClientConnectionStarted(Int64 Address, Int32 Result);
    internal delegate void ClientDeviceFound(Int64 Address, String Name);
    internal delegate void ClientDisconnected(Int64 Address, Int32 Reason);
    internal delegate void ClientValueChanged(Int64 Address, Byte[] Value);

    internal sealed class ClientWatcher : wclBluetoothLeBeaconWatcher
    {
        private const String DEVICE_NAME = "MultyGattServer";

        #region Connections management
        private Object FConnectionsCS;
        private List<GattClient> FClients; // Connected clients.
        private Dictionary<Int64, GattClient> FConnections; // Pending connections.
        private List<Int64> FFoundDevices;
        #endregion Connections management

        #region Helper method
        private void RemoveClient(GattClient Client)
        {
            lock (FConnectionsCS)
            {
                // Remove client from the connections list.
                if (FConnections.ContainsKey(Client.Address))
                {
                    Client.OnCharacteristicChanged -= ClientCharacteristicChanged;
                    Client.OnConnect -= ClientConnect;
                    Client.OnDisconnect -= ClientDisconnect;
                    FConnections.Remove(Client.Address);
                }

                // Remove client from the clients list.
                if (FClients.Contains(Client))
                    FClients.Remove(Client);
            }
        }
        #endregion Helper method

        #region Client event handlers
        private void ClientDisconnect(Object Sender, Int32 Reason)
        {
            GattClient Client = (GattClient)Sender;
            // Remove client from the lists.
            RemoveClient(Client);
            // Call disconnect event.
            DoClientDisconnected(Client.Address, Reason);
        }

        private void ClientConnect(Object Sender, Int32 Error)
        {
            GattClient Client = (GattClient)Sender;

            // If we stopped we still can get client connection event.
            if (!Monitoring)
            {
                // Remove client from connections list.
                RemoveClient(Client);
                // Disconnect client and set the connection error.
                Client.Disconnect();
                // Set the connection error code here.
                Error = wclBluetoothErrors.WCL_E_BLUETOOTH_LE_CONNECTION_TERMINATED;
            }

            // If connection failed remove client from connections list.
            if (Error != wclErrors.WCL_E_SUCCESS)
                // If it was already remove - nothing happens.
                RemoveClient(Client);
            else
            {
                // Othewrwise - add it to the connected clients list.
                lock (FConnectionsCS)
                {
                    FClients.Add(Client);
                }
            }

            // Call connection completed event.
            DoConnectionCompleted(Client.Address, Error);
        }

        private void ClientCharacteristicChanged(Object Sender, UInt16 Handle, Byte[] Value)
        {
            GattClient Client = (GattClient)Sender;
            // Simple call the value changed event.
            DoValueChanged(Client.Address, Value);
        }
        #endregion Client event handlers

        #region Events management
        private void DoClientDisconnected(Int64 Address, Int32 Reason)
        {
            if (OnClientDisconnected != null)
                OnClientDisconnected(Address, Reason);
        }

        private void DoConnectionCompleted(Int64 Address, Int32 Result)
        {
            if (OnConnectionCompleted != null)
                OnConnectionCompleted(Address, Result);
        }

        private void DoConnectionStarted(Int64 Address, Int32 Result)
        {
            if (OnConnectionStarted != null)
                OnConnectionStarted(Address, Result);
        }

        private void DoDeviceFound(Int64 Address, String Name)
        {
            if (OnDeviceFound != null)
                OnDeviceFound(Address, Name);
        }

        private void DoValueChanged(Int64 Address, Byte[] Value)
        {
            if (OnValueChanged != null)
                OnValueChanged(Address, Value);
        }
        #endregion Events management

        #region Device search handling
        protected override void DoAdvertisementFrameInformation(Int64 Address, Int64 Timestamp, SByte Rssi, String Name,
            wclBluetoothLeAdvertisementType PacketType, wclBluetoothLeAdvertisementFlag Flags)
        {
            // Do nothing if we stopped.
            if (!Monitoring)
                return;

            lock (FConnectionsCS)
            {
                // Make sure that device is not in connections list.
                if (!FConnections.ContainsKey(Address))
                {
                    // Make sure that we did not see this device early.
                    if (!FFoundDevices.Contains(Address))
                    {
                        // Check devices name.
                        if (Name == DEVICE_NAME)
                        {
                            // Add device into found list.
                            FFoundDevices.Add(Address);
                            // Call device found event.
                            DoDeviceFound(Address, Name);
                        }
                    }
                    else
                    {
                        // Device is in our devices list. Make sure we received connectable advertisement.
                        if (PacketType == wclBluetoothLeAdvertisementType.atConnectableDirected ||
                            PacketType == wclBluetoothLeAdvertisementType.atConnectableUndirected)
                        {
                            // We are ready to connect to the device.
                            GattClient Client = new GattClient();
                            // Set required event handlers.
                            Client.OnCharacteristicChanged += ClientCharacteristicChanged;
                            Client.OnConnect += ClientConnect;
                            Client.OnDisconnect += ClientDisconnect;
                            // Try to start connection to the device.
                            Int32 Result = Client.Connect(Address, Radio);
                            // Report connection start event.
                            DoConnectionStarted(Address, Result);
                            // If connection started with success...
                            if (Result == wclErrors.WCL_E_SUCCESS)
                            {
                                // ...add device to pending connections list.
                                FConnections.Add(Address, Client);
                            }

                            // Now we can remove the device from found devices list.
                            FFoundDevices.Remove(Address);
                        }
                    }
                }
            }

            base.DoAdvertisementFrameInformation(Address, Timestamp, Rssi, Name, PacketType, Flags);
        }

        protected override void DoStarted()
        {
            // Clear all lists.
            FClients.Clear();
            FConnections.Clear();
            FFoundDevices.Clear();

            base.DoStarted();
        }

        protected override void DoStopped()
        {
            List<GattClient> Clients = new List<GattClient>();
            lock (FConnectionsCS)
            {
                if (FClients.Count > 0)
                {
                    // Make copy of the connected clients.
                    foreach (GattClient Client in FClients)
                        Clients.Add(Client);
                }
            }

            // Disconnect all connected clients.
            if (Clients.Count > 0)
            {
                foreach (GattClient Client in Clients)
                    Client.Disconnect();
            }

            base.DoStopped();
        }
        #endregion Device search handling

        #region Constructor and destructor
        public ClientWatcher()
            : base()
        {
            FConnectionsCS = new Object();
            FClients = new List<GattClient>();
            FConnections = new Dictionary<Int64, GattClient>();
            FFoundDevices = new List<Int64>();

            OnClientDisconnected = null;
            OnConnectionCompleted = null;
            OnConnectionStarted = null;
            OnDeviceFound = null;
            OnValueChanged = null;
        }
        #endregion Constructor and destructor

        #region Communication methods
        public Int32 Disconnect(Int64 Address)
        {
            if (!Monitoring)
                return wclConnectionErrors.WCL_E_CONNECTION_CLOSED;

            GattClient Client = null;
            lock (FConnectionsCS)
            {
                if (FConnections.ContainsKey(Address))
                    Client = FConnections[Address];
                if (Client != null)
                {
                    if (!FClients.Contains(Client))
                        Client = null;
                }
            }

            if (Client == null)
                return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE;
            return Client.Disconnect();
        }

        public Int32 ReadData(Int64 Address, out Byte[] Data)
        {
            Data = null;

            if (!Monitoring)
                return wclConnectionErrors.WCL_E_CONNECTION_CLOSED;

            lock (FConnectionsCS)
            {
                if (!FConnections.ContainsKey(Address))
                    return wclBluetoothErrors.WCL_E_BLUETOOTH_LE_DEVICE_NOT_FOUND;

                GattClient Client = FConnections[Address];
                if (!FClients.Contains(Client))
                    return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE;

                return Client.ReadValue(out Data);
            }
        }

        public Int32 WriteData(Int64 Address, Byte[] Data)
        {
            if (!Monitoring)
                return wclConnectionErrors.WCL_E_CONNECTION_CLOSED;
            if (Data == null || Data.Length == 0)
                return wclErrors.WCL_E_INVALID_ARGUMENT;

            lock (FConnectionsCS)
            {
                if (!FConnections.ContainsKey(Address))
                    return wclBluetoothErrors.WCL_E_BLUETOOTH_LE_DEVICE_NOT_FOUND;

                GattClient Client = FConnections[Address];
                if (!FClients.Contains(Client))
                    return wclConnectionErrors.WCL_E_CONNECTION_NOT_ACTIVE;

                return Client.WriteValue(Data);
            }
        }
        #endregion Communication methods

        #region Events
        public event ClientDisconnected OnClientDisconnected;
        public event ClientConnectionCompleted OnConnectionCompleted;
        public event ClientConnectionStarted OnConnectionStarted;
        public event ClientDeviceFound OnDeviceFound;
        public event ClientValueChanged OnValueChanged;
        #endregion Events
    };
}
