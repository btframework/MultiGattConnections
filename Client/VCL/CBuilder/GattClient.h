//---------------------------------------------------------------------------

#ifndef GattClientH
#define GattClientH
//---------------------------------------------------------------------------

#include "wclBluetooth.hpp"

const GUID SERVICE_UUID = {0xa25fede7, 0x395c, 0x4241, {0xad, 0xc0, 0x48, 0x10, 0x04, 0xd8, 0x19, 0x00}};
const GUID NOTIFIABLE_CHARACTERISTIC_UUID = {0xeabbbb91, 0xb7e5, 0x4e50, {0xb7, 0xaa, 0xce, 0xd2, 0xbe, 0x8d, 0xfb, 0xbe}};
const GUID READABLE_CHARACTERISTIC_UUID = {0x468dfe19, 0x8de3, 0x4181, {0xb7, 0x28, 0x09, 0x02, 0xc5, 0x0a, 0x5e, 0x6d}};
const GUID WRITABLE_CHARACTERISTIC_UUID = {0x421754b0, 0xe70a, 0x42c9, {0x90, 0xed, 0x4a, 0xed, 0x82, 0xfa, 0x7a, 0xc0}};

class TGattClient : public TwclGattClient
{
private:
	bool					FConnected;
	RTL_CRITICAL_SECTION	FCS;

	TwclGattCharacteristic	FReadableChar;
	TwclGattCharacteristic	FWritableChar;

protected:
	// The method called when connection procedure completed (with or without
	// success). If the Error parameter is WCL_E_SUCCESS then we are connected
	// and can read attributes and subscribe. If something goes wrong during
	// attributes reading and subscribing we have to disconnect from the device.
	// If the Error parameter is not WCL_E_SUCCESS then connection failed.
	// Simple call OnConnect event with error and do nothing more.
	virtual void __fastcall DoConnect(const int Error);
	// The method called when the remote device disconnected. The Reason
	// parameter indicates disconnection reason.
	virtual void __fastcall DoDisconnect(const int Reason);

public:
	virtual __fastcall TGattClient();
	virtual __fastcall ~TGattClient();

	// Override connect method. We need it for thread synchronization.
	int __fastcall Connect(const __int64 Address, TwclBluetoothRadio* Radio);
	// Override disconnect method. We need it for thread synchronization.
	int __fastcall Disconnect();

	// Simple read value from the readable characteristic.
	int __fastcall ReadValue(TwclGattCharacteristicValue& Value);
	// Simple write value to the writable characteristic.
	int __fastcall WriteValue(const TwclGattCharacteristicValue& Value);
};

#endif
