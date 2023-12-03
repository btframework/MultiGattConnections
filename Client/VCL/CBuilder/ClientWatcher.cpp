//---------------------------------------------------------------------------

#pragma hdrstop

#include <algorithm>

#include "ClientWatcher.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

/* TClientWatcher */

TGattClient* __fastcall TClientWatcher::FindClient(const __int64 Address)
{
	for (CLIENTS_LIST::iterator i = FClients->begin(); i != FClients->end(); i++)
	{
		if ((*i)->Address == Address)
			return *i;
	}
	return NULL;
}

TGattClient* __fastcall TClientWatcher::FindConnection(const __int64 Address)
{
	for (CLIENTS_LIST::iterator i = FConnections->begin(); i != FConnections->end(); i++)
	{
		if ((*i)->Address == Address)
			return *i;
	}
	return NULL;
}

int __fastcall TClientWatcher::ReadData(const __int64 Address,
	TwclGattCharacteristicValue& Data)
{
	Data.Length = 0;

	if (!Monitoring)
		return WCL_E_CONNECTION_CLOSED;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		TGattClient* Client = FindClient(Address);
		if (Client == NULL)
			return WCL_E_CONNECTION_NOT_ACTIVE;
		return Client->ReadValue(Data);
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

void __fastcall TClientWatcher::RemoveClient(TGattClient* Client)
{
	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		// Remove client from the connections list.
		if (FindConnection(Client->Address) != NULL)
		{
			Client->OnCharacteristicChanged = NULL;
			Client->OnConnect = NULL;
			Client->OnDisconnect = NULL;
			FConnections->remove(Client);
		}

		// Remove client from the clients list.
		if (FindClient(Client->Address))
			FClients->remove(Client);
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}

	DestroyClient(Client);
}

int __fastcall TClientWatcher::WriteData(const __int64 Address,
	const TwclGattCharacteristicValue& Data)
{
	if (!Monitoring)
		return WCL_E_CONNECTION_CLOSED;
	if (Data.Length == 0)
		return WCL_E_INVALID_ARGUMENT;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		TGattClient* Client = FindClient(Address);
		if (Client == NULL)
			return WCL_E_CONNECTION_NOT_ACTIVE;
		return Client->WriteValue(Data);
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

void __fastcall TClientWatcher::ClientCharacteristicChanged(TObject* Sender,
	const unsigned short Handle, const TwclGattCharacteristicValue Value)
{
	TGattClient* Client = (TGattClient*)Sender;
	// Simple call the value changed event.
	DoValueChanged(Client->Address, Value);
}

void __fastcall TClientWatcher::ClientConnect(TObject* Sender, const int Error)
{
	TGattClient* Client = (TGattClient*)Sender;
	__int64 Address = Client->Address;
	// We need it to be able to change the value.
	int Res = Error;

	// If we stopped we still can get client connection event.
	if (!Monitoring)
	{
		// Set the connection error code here.
		Res = WCL_E_BLUETOOTH_LE_CONNECTION_TERMINATED;
		// Disconnect client and set the connection error.
		Client->Disconnect();
	}
	else
	{
		// If connection failed remove client from connections list.
		if (Res != WCL_E_SUCCESS)
			// Remove client from the lists.
			RemoveClient(Client);
		else
		{
			// Othewrwise - add it to the connected clients list.
			EnterCriticalSection(&FConnectionsCS);
			__try
			{
				FClients->push_back(Client);
			}
			__finally
			{
				LeaveCriticalSection(&FConnectionsCS);
			}
		}
	}

	// Call connection completed event.
	DoConnectionCompleted(Address, Res);
}

void __fastcall TClientWatcher::ClientDisconnect(TObject* Sender,
	const int Reason)
{
	TGattClient* Client = (TGattClient*)Sender;
	__int64 Address = Client->Address;

	// Remove client from the lists.
	RemoveClient(Client);
	// Call disconnect event.
	DoClientDisconnected(Address, Reason);
}

__fastcall TClientWatcher::TClientWatcher() : TwclBluetoothLeBeaconWatcher(NULL)
{
	InitializeCriticalSection(&FConnectionsCS);

	FClients = new CLIENTS_LIST();
	FConnections = new CLIENTS_LIST();
	FFoundDevices = new FOUND_DEVICES_LIST();

	FTempClient = NULL;

	FOnClientDisconnected = NULL;
	FOnConnectionCompleted = NULL;
	FOnConnectionStarted = NULL;
	FOnDeviceFound = NULL;
	FOnValueChanged = NULL;
}

__fastcall TClientWatcher::~TClientWatcher()
{
	// We have to stop first! If we did not do it here than inherited destructor
	// calls Stop() and we wil crash because all objects are destroyed!
	Stop();

	DestroyClient(NULL);

	// Now we can destroy the objects.
	DeleteCriticalSection(&FConnectionsCS);

	delete FClients;
	delete FConnections;
	delete FFoundDevices;
}

void __fastcall TClientWatcher::DestroyClient(TGattClient* Client)
{
	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		if (FTempClient != NULL)
		{
			FTempClient->Free();
			FTempClient = NULL;
		}

		FTempClient = Client;
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

int __fastcall TClientWatcher::Disconnect(const __int64 Address)
{
	if (!Monitoring)
		return WCL_E_CONNECTION_CLOSED;

	TGattClient* Client = NULL;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		Client = FindClient(Address);
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}

	if (Client == NULL)
		return WCL_E_CONNECTION_NOT_ACTIVE;
	return Client->Disconnect();
}

void __fastcall TClientWatcher::DoAdvertisementFrameInformation(
	const __int64 Address, const __int64 Timestamp, const signed char Rssi,
	const System::UnicodeString Name,
	const TwclBluetoothLeAdvertisementType PacketType,
	const TwclBluetoothLeAdvertisementFlags Flags)
{
	if (Monitoring)
	{
		EnterCriticalSection(&FConnectionsCS);
		__try
		{
			// Make sure that device is not in connections list.
			if (FindConnection(Address) == NULL)
			{
				// Make sure that we did not see this device early.
				if (std::find(FFoundDevices->begin(), FFoundDevices->end(), Address) == FFoundDevices->end())
				{
					// Check devices name.
					if (Name == DEVICE_NAME)
					{
						// Add device into found list.
						FFoundDevices->push_back(Address);
						// Call device found event.
						DoDeviceFound(Address, Name);
					}
				}
				else
				{
					// Device is in our devices list. Make sure we received
					// connectable advertisement.
					if (PacketType == atConnectableDirected || PacketType == atConnectableUndirected)
					{
						// We are ready to connect to the device.
						TGattClient* Client = new TGattClient();
						// Set required event handlers.
						Client->OnCharacteristicChanged = ClientCharacteristicChanged;
						Client->OnConnect = ClientConnect;
						Client->OnDisconnect = ClientDisconnect;
						// Try to start connection to the device.
						int Result = Client->Connect(Address, Radio);
						// Report connection start event.
						DoConnectionStarted(Address, Result);

						// If connection started with success...
						if (Result == WCL_E_SUCCESS)
							// ...add device to pending connections list.
							FConnections->push_back(Client);
						else
							// Otherwise - destroy the object.
							Client->Free();

						// Now we can remove the device from found devices list.
						FFoundDevices->remove(Address);
					}
				}
			}
		}
		__finally
		{
			LeaveCriticalSection(&FConnectionsCS);
		}

		TwclBluetoothLeBeaconWatcher::DoAdvertisementFrameInformation(Address,
		  Timestamp, Rssi, Name, PacketType, Flags);
	}
}

void __fastcall TClientWatcher::DoClientDisconnected(const __int64 Address,
	const int Reason)
{
	if (FOnClientDisconnected != NULL)
		FOnClientDisconnected(Address, Reason);
}

void __fastcall TClientWatcher::DoConnectionCompleted(const __int64 Address,
	const int Result)
{
	if (FOnConnectionCompleted != NULL)
		FOnConnectionCompleted(Address, Result);
}

void __fastcall TClientWatcher::DoConnectionStarted(const __int64 Address,
	const int Result)
{
	if (FOnConnectionStarted != NULL)
		FOnConnectionStarted(Address, Result);
}

void __fastcall TClientWatcher::DoDeviceFound(const __int64 Address,
	const System::UnicodeString Name)
{
	if (FOnDeviceFound != NULL)
		FOnDeviceFound(Address, Name);
}

void __fastcall TClientWatcher::DoStarted()
{
	// Clear all lists.
	FClients->clear();
	FConnections->clear();
	FFoundDevices->clear();

	TwclBluetoothLeBeaconWatcher::DoStarted();
}

void __fastcall TClientWatcher::DoStopped()
{
	CLIENTS_LIST* Clients = new CLIENTS_LIST();

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		if (FClients->size() > 0)
		{
			// Make copy of the connected clients.
			for (CLIENTS_LIST::iterator i = FClients->begin(); i != FClients->end(); i++)
				Clients->push_back(*i);
		}
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}

	// Disconnect all connected clients.
	if (Clients->size() > 0)
	{
		for (CLIENTS_LIST::iterator i = Clients->begin(); i != Clients->end(); i++)
			(*i)->Disconnect();
	}

	delete Clients;

	DestroyClient(NULL);

	TwclBluetoothLeBeaconWatcher::DoStopped();
}

void __fastcall TClientWatcher::DoValueChanged(const __int64 Address,
	const TwclGattCharacteristicValue& Value)
{
	if (FOnValueChanged != NULL)
		FOnValueChanged(Address, Value);
}
