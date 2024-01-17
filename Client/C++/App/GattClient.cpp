#include "pch.h"

#include "GattClient.h"

void CGattClient::DoConnect(const int Error)
{
	// Copy this to be able to modify it if needed.
	int Res = Error;

	// If connection was success try to read required attributes.
	if (Res == WCL_E_SUCCESS)
	{
		wclGattUuid Uuid;
		Uuid.IsShortUuid = false;

		// First find required service.
		Uuid.LongUuid = SERVICE_UUID;
		wclGattService Service;
		Res = FindService(Uuid, Service);
		if (Res == WCL_E_SUCCESS)
		{
			// Service found. Try to find readable characteristic.
			Uuid.LongUuid = READABLE_CHARACTERISTIC_UUID;
			Res = FindCharacteristic(Service, Uuid, FReadableChar);
			if (Res == WCL_E_SUCCESS)
			{
				// If readable characteristic found try to find writable one.
				Uuid.LongUuid = WRITABLE_CHARACTERISTIC_UUID;
				Res = FindCharacteristic(Service, Uuid, FWritableChar);
				if (Res == WCL_E_SUCCESS)
				{
					// Ok, we got writable characteristic. Now try to find notifiable one.
					Uuid.LongUuid = NOTIFIABLE_CHARACTERISTIC_UUID;
					wclGattCharacteristic NotifiableChar;
					Res = FindCharacteristic(Service, Uuid, NotifiableChar);
					if (Res == WCL_E_SUCCESS)
					{
						// Notifiable characteristic found. Try to subscribe. We do not need to
						// save the characteristic globally. It will be unsubscribed during disconnection.
						Res = SubscribeForNotifications(NotifiableChar);

						// If subscribed - set connected flag.
						if (Res == WCL_E_SUCCESS)
						{
							EnterCriticalSection(&FCS);
							__try
							{
								FConnected = true;
							}
							__finally
							{
								LeaveCriticalSection(&FCS);
							}
						}
					}
				}
			}
		}

		// If something went wrong we must disconnect!
		if (Res != WCL_E_SUCCESS)
			Disconnect();
	}

	// Call inherited method anyway so the OnConnect event fires.
	CwclGattClient::DoConnect(Res);
}

void CGattClient::DoDisconnect(const int Reason)
{
	// Make sure that we were "connected".
	if (FConnected)
	{
		// Simple cleanup.
		EnterCriticalSection(&FCS);
		__try
		{
			FConnected = false;
		}
		__finally
		{
			LeaveCriticalSection(&FCS);
		}
	}

	// Call the inherited method to fire the OnDisconnect event.
	CwclGattClient::DoDisconnect(Reason);
}

CGattClient::CGattClient() : CwclGattClient()
{
	FConnected = false;

	InitializeCriticalSection(&FCS);
}

CGattClient::~CGattClient()
{
	Disconnect();

	DeleteCriticalSection(&FCS);
}

// Override connect method. We need it for thread synchronization.
int CGattClient::Connect(const __int64 Address, CwclBluetoothRadio* const Radio)
{
	if (Address == 0 || Radio == NULL)
		return WCL_E_INVALID_ARGUMENT;

	EnterCriticalSection(&FCS);
	__try
	{
		if (FConnected || State != csDisconnected)
			return WCL_E_CONNECTION_ACTIVE;

		this->Address = Address;
		return CwclGattClient::Connect(Radio);
	}
	__finally
	{
		LeaveCriticalSection(&FCS);
	}
}

// Override disconnect method. We need it for thread synchronization.
int CGattClient::Disconnect()
{
	EnterCriticalSection(&FCS);
	__try
	{
		if (!FConnected)
			return WCL_E_CONNECTION_NOT_ACTIVE;

		return CwclGattClient::Disconnect();
	}
	__finally
	{ 
		LeaveCriticalSection(&FCS);
	}
}

int CGattClient::ReadValue(unsigned char*& Value, unsigned long& Length)
{
	Value = NULL;
	Length = 0;

	EnterCriticalSection(&FCS);
	__try
	{
		if (!FConnected)
			return WCL_E_CONNECTION_CLOSED;

		// Always use Read From Device.
		return ReadCharacteristicValue(FReadableChar, goReadFromDevice, Value, Length);
	}
	__finally
	{
		LeaveCriticalSection(&FCS);
	}
}

int CGattClient::WriteValue(const unsigned char* const Value, const unsigned long Length)
{
	if (Value == NULL || Length == 0)
		return WCL_E_INVALID_ARGUMENT;

	EnterCriticalSection(&FCS);
	__try
	{
		if (!FConnected)
			return WCL_E_CONNECTION_CLOSED;

		return WriteCharacteristicValue(FWritableChar, Value, Length);
	}
	__finally
	{
		LeaveCriticalSection(&FCS);
	}
}