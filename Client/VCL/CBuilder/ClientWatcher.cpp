//---------------------------------------------------------------------------

#pragma hdrstop

#include <algorithm>

#include "ClientWatcher.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

/* TClientWatcher */

void __fastcall TClientWatcher::ClientCharacteristicChanged(TObject* Sender,
	const unsigned short Handle, const TwclGattCharacteristicValue Value)
{
	if (Monitoring)
	{
		TGattClient* Client = (TGattClient*)Sender;
		// Simple call the value changed event.
		DoValueChanged(Client->Address, Value);
	}
}

void __fastcall TClientWatcher::ClientConnect(TObject* Sender, const int Error)
{
	TGattClient* Client = (TGattClient*)Sender;

	// If we stopped we still can get client connection event.
	if (!Monitoring)
	{
		// Disconnect client and set the connection error.
		if (Error == WCL_E_SUCCESS)
			Client->Disconnect();
		else
			RemoveClient(Client);
		return;
	}

	// If connection failed remove client from connections list.
	if (Error != WCL_E_SUCCESS)
		RemoveClient(Client);
	else
	{
		// Othewrwise - add it to the connected clients list.
		EnterCriticalSection(&FConnectionsCS);
		__try
		{
			FConnections->push_back(Client);
		}
		__finally
		{
			LeaveCriticalSection(&FConnectionsCS);
		}
	}

	// Call connection completed event.
	DoConnectionCompleted(Client->Address, Error);
}

void __fastcall TClientWatcher::ClientDisconnect(TObject* Sender,
	const int Reason)
{
	TGattClient* Client = (TGattClient*)Sender;
	// Call disconnect event.
	DoClientDisconnected(Client->Address, Reason);
	// Remove client from the lists.
	RemoveClient(Client);
}

__fastcall TClientWatcher::TClientWatcher() : TwclBluetoothLeBeaconWatcher(NULL)
{
	InitializeCriticalSection(&FConnectionsCS);

	FConnections = new std::list<TGattClient*>();
	FPendingConnections = new std::list<__int64>();
	FOldClient = NULL;

	OnClientDisconnected = NULL;
	OnConnectionCompleted = NULL;
	OnConnectionStarted = NULL;
	OnDeviceFound = NULL;
	OnValueChanged = NULL;
}

__fastcall TClientWatcher::~TClientWatcher()
{
	// We have to call stop here to prevent from issues with objects!
	Stop();

	delete FConnections;
	delete FPendingConnections;

	if (FOldClient != NULL)
		FOldClient->Free();

	DeleteCriticalSection(&FConnectionsCS);
}

__fastcall int TClientWatcher::Disconnect(const __int64 Address)
{
	if (!Monitoring)
		return WCL_E_CONNECTION_CLOSED;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		TGattClient* Client = FindClient(Address);
		if (Client == NULL)
			return WCL_E_CONNECTION_NOT_ACTIVE;
		return Client->Disconnect();
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

void __fastcall TClientWatcher::DoAdvertisementFrameInformation(
	const __int64 Address, const __int64 Timestamp, const signed char Rssi,
	const System::UnicodeString Name,
	const TwclBluetoothLeAdvertisementType PacketType,
	const TwclBluetoothLeAdvertisementFlags Flags)
{
	// Do nothing if we stopped.
	if (!Monitoring)
		return;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		// Make sure that device is not in connections list.
		if (std::find(FPendingConnections->begin(), FPendingConnections->end(), Address) != FPendingConnections->end())
			return;

		// Check devices name.
		if (Name == DEVICE_NAME)
		{
			// Notify about new device.
			DoDeviceFound(Address, Name);

			// Create client.
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
				FPendingConnections->push_back(Address);
			else
				Client->Free();
		}
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
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

void __fastcall TClientWatcher::DoStopped()
{
	std::list<TGattClient*> Clients;
	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		if (FConnections->size() > 0)
		{
			// Make copy of the connected clients.
			for (std::list<TGattClient*>::iterator Client = FConnections->begin(); Client != FConnections->end(); Client++)
				Clients.push_back(*Client);
		}
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}

	// Disconnect all connected clients.
	if (Clients.size() > 0)
	{
		for (std::list<TGattClient*>::iterator Client = Clients.begin(); Client != Clients.end(); Client++)
			(*Client)->Disconnect();
	}

	TwclBluetoothLeBeaconWatcher::DoStopped();
}

void __fastcall TClientWatcher::DoValueChanged(const __int64 Address,
	const TwclGattCharacteristicValue Value)
{
	if (FOnValueChanged != NULL)
		FOnValueChanged(Address, Value);
}

TGattClient* __fastcall TClientWatcher::FindClient(const __int64 Address)
{
	if (FConnections->size() > 0)
	{
		for (std::list<TGattClient*>::iterator Client = FConnections->begin(); Client != FConnections->end(); Client++)
		{
			if ((*Client)->Address == Address)
				return *Client;
		}
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
	if (Client == NULL)
		return;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		// Remove client from the connections list.
		if (std::find(FPendingConnections->begin(), FPendingConnections->end(), Client->Address) != FPendingConnections->end())
		{
			Client->OnCharacteristicChanged = NULL;
			Client->OnConnect = NULL;
			Client->OnDisconnect = NULL;
			FPendingConnections->remove(Client->Address);
		}

		// Remove client from the clients list.
		if (std::find(FConnections->begin(), FConnections->end(), Client) != FConnections->end())
			FConnections->remove(Client);

		SetOldClient(Client);
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

void __fastcall TClientWatcher::SetOldClient(TGattClient* Client)
{
	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		if (FOldClient != NULL)
		{
			FOldClient->Free();
			FOldClient = NULL;
		}

		FOldClient = Client;
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
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
