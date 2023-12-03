//---------------------------------------------------------------------------

#ifndef ClientWatcherH
#define ClientWatcherH
//---------------------------------------------------------------------------

#include <list>
#include <map>

#include "wclBluetooth.hpp"
#include "GattClient.h"

typedef void __fastcall (__closure *TClientConnectionCompleted)(
	const __int64 Address, const int Result);
typedef void __fastcall (__closure *TClientConnectionStarted)(
	const __int64 Address, const int Result);
typedef void __fastcall (__closure *TClientDeviceFound)(const __int64 Address,
	const System::UnicodeString Name);
typedef void __fastcall (__closure *TClientDisconnected)(const __int64 Address,
	const int Reason);
typedef void __fastcall (__closure *TClientValueChanged)(const __int64 Address,
	const TwclGattCharacteristicValue& Value);

const System::UnicodeString DEVICE_NAME = "MultyGattServer";

class TClientWatcher : public TwclBluetoothLeBeaconWatcher
{
private:
	typedef std::list<TGattClient*> CLIENTS_LIST;
	typedef std::list<__int64> FOUND_DEVICES_LIST;

	RTL_CRITICAL_SECTION		FConnectionsCS;
	CLIENTS_LIST*				FConnections;
	CLIENTS_LIST*				FPendingConnections;
	FOUND_DEVICES_LIST*			FFoundDevices;
	TGattClient*				FTempClient; // Used to store client that must be destroyed.

	TClientDisconnected			FOnClientDisconnected;
	TClientConnectionCompleted	FOnConnectionCompleted;
	TClientConnectionStarted	FOnConnectionStarted;
	TClientDeviceFound			FOnDeviceFound;
	TClientValueChanged			FOnValueChanged;

	TGattClient* __fastcall FindClient(const __int64 Address);
	TGattClient* __fastcall FindConnection(const __int64 Address);

	void __fastcall DestroyClient(TGattClient* Client);
	void __fastcall RemoveClient(TGattClient* Client);

	void __fastcall ClientDisconnect(TObject* Sender, const int Reason);
	void __fastcall ClientConnect(TObject* Sender, const int Error);
	void __fastcall ClientCharacteristicChanged(TObject* Sender,
		const unsigned short Handle, const TwclGattCharacteristicValue Value);

	void __fastcall DoClientDisconnected(const __int64 Address,
		const int Reason);
	void __fastcall DoConnectionCompleted(const __int64 Address,
		const int Result);
	void __fastcall DoConnectionStarted(const __int64 Address,
		const int Result);
	void __fastcall DoDeviceFound(const __int64 Address,
		const System::UnicodeString Name);
	void __fastcall DoValueChanged(const __int64 Address,
		const TwclGattCharacteristicValue& Value);

protected:
	virtual void __fastcall DoAdvertisementFrameInformation(
		const __int64 Address, const __int64 Timestamp, const signed char Rssi,
		const System::UnicodeString Name,
		const TwclBluetoothLeAdvertisementType PacketType,
		const TwclBluetoothLeAdvertisementFlags Flags);
	virtual void __fastcall DoStarted();
	virtual void __fastcall DoStopped();

public:
	virtual __fastcall TClientWatcher();
	virtual __fastcall ~TClientWatcher();

	int __fastcall Disconnect(const __int64 Address);
	int __fastcall ReadData(const __int64 Address,
		TwclGattCharacteristicValue& Data);
	int __fastcall WriteData(const __int64 Address,
		const TwclGattCharacteristicValue& Data);

	__property TClientDisconnected OnClientDisconnected = {
		read = FOnClientDisconnected, write = FOnClientDisconnected };
	__property TClientConnectionCompleted OnConnectionCompleted = {
		read = FOnConnectionCompleted, write = FOnConnectionCompleted };
	__property TClientConnectionStarted OnConnectionStarted = {
		read = FOnConnectionStarted, write = FOnConnectionStarted };
	__property TClientDeviceFound OnDeviceFound = { read = FOnDeviceFound,
		write = FOnDeviceFound };
	__property TClientValueChanged OnValueChanged = { read = FOnValueChanged,
		write = FOnValueChanged };
};

#endif
