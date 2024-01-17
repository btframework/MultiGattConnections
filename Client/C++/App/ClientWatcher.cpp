#include "pch.h"

#include "ClientWatcher.h"

void CClientWatcher::DoClientDisconnected(const __int64 Address, const int Reason)
{
	__raise OnClientDisconnected(Address, Reason);
}

void CClientWatcher::DoConnectionCompleted(const __int64 Address, const int Result)
{
	__raise OnConnectionCompleted(Address, Result);
}

void CClientWatcher::DoConnectionStarted(const __int64 Address, const int Result)
{
	__raise OnConnectionStarted(Address, Result);
}

void CClientWatcher::DoDeviceFound(const __int64 Address, const tstring& Name)
{
	__raise OnDeviceFound(Address, Name);
}

void CClientWatcher::DoValueChanged(const __int64 Address, const unsigned char* Value,
	const unsigned long Length)
{
	__raise OnValueChanged(Address, Value, Length);
}

void CClientWatcher::ClientCharacteristicChanged(void* Sender, const unsigned short Handle,
	const unsigned char* Value, const unsigned long Length)
{
	if (Monitoring)
	{
		CGattClient* Client = (CGattClient*)Sender;
		// Simple call the value changed event.
		DoValueChanged(Client->Address, Value, Length);
	}
}

void CClientWatcher::ClientConnect(void* Sender, const int Error)
{
	CGattClient* Client = (CGattClient*)Sender;

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

void CClientWatcher::ClientDisconnect(void* Sender, const int Reason)
{
	CGattClient* Client = (CGattClient*)Sender;
	// Call disconnect event.
	DoClientDisconnected(Client->Address, Reason);
	// Remove client from the lists.
	RemoveClient(Client);
}

CClientWatcher::CClientWatcher() : CwclBluetoothLeBeaconWatcher()
{
	InitializeCriticalSection(&FConnectionsCS);

	FConnections = new list<CGattClient*>();
	FPendingConnections = new list<__int64>();
	FOldClient = NULL;
}

CClientWatcher::~CClientWatcher()
{
	// We have to call stop here to prevent from issues with objects!
	Stop();

	delete FConnections;
	delete FPendingConnections;

	if (FOldClient != NULL)
		delete FOldClient;

	DeleteCriticalSection(&FConnectionsCS);
}

int CClientWatcher::Disconnect(const __int64 Address)
{
	if (!Monitoring)
		return WCL_E_CONNECTION_CLOSED;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		CGattClient* Client = FindClient(Address);
		if (Client == NULL)
			return WCL_E_CONNECTION_NOT_ACTIVE;
		return Client->Disconnect();
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

CGattClient* CClientWatcher::FindClient(const __int64 Address)
{
	if (FConnections->size() > 0)
	{
		for (list<CGattClient*>::iterator Client = FConnections->begin(); Client != FConnections->end(); Client++)
		{
			if ((*Client)->Address == Address)
				return *Client;
		}
	}
	return NULL;
}

void CClientWatcher::RemoveClient(CGattClient* Client)
{
	if (Client == NULL)
		return;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		// Remove client from the connections list.
		if (find(FPendingConnections->begin(), FPendingConnections->end(), Client->Address) != FPendingConnections->end())
		{
			__unhook(Client);
			FPendingConnections->remove(Client->Address);
		}

		// Remove client from the clients list.
		if (find(FConnections->begin(), FConnections->end(), Client) != FConnections->end())
			FConnections->remove(Client);

		SetOldClient(Client);
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

void CClientWatcher::SetOldClient(CGattClient* Client)
{
	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		if (FOldClient != NULL)
		{
			delete FOldClient;
			FOldClient = NULL;
		}

		FOldClient = Client;
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

void CClientWatcher::CopyClients(list<CGattClient*>* Clients)
{
	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		if (FConnections->size() > 0)
		{
			// Make copy of the connected clients.
			for (list<CGattClient*>::iterator Client = FConnections->begin(); Client != FConnections->end(); Client++)
				Clients->push_back(*Client);
		}
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

void CClientWatcher::DoStopped()
{
	list<CGattClient*>* Clients = new list<CGattClient*>();
	CopyClients(Clients);

	// Disconnect all connected clients.
	if (Clients->size() > 0)
	{
		for (list<CGattClient*>::iterator Client = Clients->begin(); Client != Clients->end(); Client++)
			(*Client)->Disconnect();
	}

	delete Clients;

	CwclBluetoothLeBeaconWatcher::DoStopped();
}

void CClientWatcher::CreateClient(const __int64 Address)
{
	// Create client.
	CGattClient* Client = new CGattClient();
	// Set required event handlers.
	__hook(&CGattClient::OnCharacteristicChanged, Client, &CClientWatcher::ClientCharacteristicChanged);
	__hook(&CGattClient::OnConnect, Client, &CClientWatcher::ClientConnect);
	__hook(&CGattClient::OnDisconnect, Client, &CClientWatcher::ClientDisconnect);
	// Try to start connection to the device.
	int Result = Client->Connect(Address, Radio);
	// Report connection start event.
	DoConnectionStarted(Address, Result);
	// If connection started with success...
	if (Result == WCL_E_SUCCESS)
		// ...add device to pending connections list.
		FPendingConnections->push_back(Address);
	else
		delete Client;
}

void CClientWatcher::DoAdvertisementFrameInformation(const __int64 Address, const __int64 Timestamp,
	const char Rssi, const tstring& Name, const wclBluetoothLeAdvertisementType PacketType,
	const wclBluetoothLeAdvertisementFlags& Flags)
{
	// Do nothing if we stopped.
	if (!Monitoring)
		return;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		// Make sure that device is not in connections list.
		if (find(FPendingConnections->begin(), FPendingConnections->end(), Address) != FPendingConnections->end())
			return;

		// Check devices name.
		if (Name == DEVICE_NAME)
		{
			// Notify about new device.
			DoDeviceFound(Address, Name);
			CreateClient(Address);
		}
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

int CClientWatcher::ReadData(const __int64 Address, unsigned char*& Data, unsigned long& Length)
{
	Data = NULL;
	Length = 0;

	if (!Monitoring)
		return WCL_E_CONNECTION_CLOSED;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		CGattClient* Client = FindClient(Address);
		if (Client == NULL)
			return WCL_E_CONNECTION_NOT_ACTIVE;
		return Client->ReadValue(Data, Length);
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}

int CClientWatcher::WriteData(const __int64 Address, const unsigned char* const Data,
	const unsigned long Length)
{
	if (!Monitoring)
		return WCL_E_CONNECTION_CLOSED;

	if (Data == NULL || Length == 0)
		return WCL_E_INVALID_ARGUMENT;

	EnterCriticalSection(&FConnectionsCS);
	__try
	{
		CGattClient* Client = FindClient(Address);
		if (Client == NULL)
			return WCL_E_CONNECTION_NOT_ACTIVE;
		return Client->WriteValue(Data, Length);
	}
	__finally
	{
		LeaveCriticalSection(&FConnectionsCS);
	}
}