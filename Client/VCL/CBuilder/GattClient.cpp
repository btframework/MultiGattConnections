//---------------------------------------------------------------------------

#pragma hdrstop

#include "GattClient.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

/* TGattClient */

int __fastcall TGattClient::Connect(const __int64 Address,
  TwclBluetoothRadio* Radio)
{
	if (Address == 0 || Radio == NULL)
		return WCL_E_INVALID_ARGUMENT;

	EnterCriticalSection(&FCS);
	__try
	{
		if (FConnected || State != csDisconnected)
			return WCL_E_CONNECTION_ACTIVE;

		this->Address = Address;
		return TwclGattClient::Connect(Radio);
	}
	__finally
	{
	  LeaveCriticalSection(&FCS);
	}
}

__fastcall TGattClient::TGattClient() : TwclGattClient(NULL)
{
	FConnected = false;
	InitializeCriticalSection(&FCS);
}

__fastcall TGattClient::~TGattClient()
{
	// We have to call disconnect here, otherwise when inherited destructor
	// will be called we will get crash because all the objects are already
	// destroyed.
	Disconnect();

	// Now destroy the objects.
	DeleteCriticalSection(&FCS);
}

int __fastcall TGattClient::Disconnect()
{
	EnterCriticalSection(&FCS);
	__try
	{
		if (!FConnected)
			return WCL_E_CONNECTION_NOT_ACTIVE;
		return TwclGattClient::Disconnect();
	}
	__finally
	{
		LeaveCriticalSection(&FCS);
	}
}

void __fastcall TGattClient::DoConnect(const int Error)
{
	// Copy it so we can change it.
	int Res = Error;
	// If connection was success try to read required attributes.
	if (Res == WCL_E_SUCCESS)
	{
		TwclGattUuid Uuid;
		Uuid.IsShortUuid = false;

		// First find required service.
		Uuid.LongUuid = SERVICE_UUID;
		TwclGattService Service;
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
					// Ok, we got writable characteristic. Now try to find
					// notifiable one.
					Uuid.LongUuid = NOTIFIABLE_CHARACTERISTIC_UUID;
					TwclGattCharacteristic NotifiableChar;
					Res = FindCharacteristic(Service, Uuid, NotifiableChar);
					if (Res == WCL_E_SUCCESS)
					{
						// Notifiable characteristic found. Try to subscribe. We
						// do not need to save the characteristic globally. It
						// will be unsubscribed during disconnection.
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
	TwclGattClient::DoConnect(Res);
}

void __fastcall TGattClient::DoDisconnect(const int Reason)
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
	TwclGattClient::DoDisconnect(Reason);
}

int __fastcall TGattClient::ReadValue(TwclGattCharacteristicValue& Value)
{
	Value.Length = 0;

	EnterCriticalSection(&FCS);
	__try
	{
		if (!FConnected)
			return WCL_E_CONNECTION_CLOSED;
		// Always use Read From Device.
		return ReadCharacteristicValue(FReadableChar, goReadFromDevice, Value);
	}
	__finally
	{
		LeaveCriticalSection(&FCS);
	}
}

int __fastcall TGattClient::WriteValue(const TwclGattCharacteristicValue& Value)
{
	if (Value.Length == 0)
		return WCL_E_INVALID_ARGUMENT;

	EnterCriticalSection(&FCS);
	__try
	{
		if (!FConnected)
			return WCL_E_CONNECTION_CLOSED;
		return WriteCharacteristicValue(FWritableChar, Value);
	}
	__finally
	{
		LeaveCriticalSection(&FCS);
	}
}
