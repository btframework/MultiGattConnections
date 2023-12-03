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
	#pragma region Connections management
	RTL_CRITICAL_SECTION		FConnectionsCS;
	std::list<TGattClient*>*	FConnections;
	std::list<__int64>*			FPendingConnections;
	TGattClient*				FOldClient;
	#pragma end_region Connections management

	#pragma region Events
	TClientDisconnected			FOnClientDisconnected;
	TClientConnectionCompleted	FOnConnectionCompleted;
	TClientConnectionStarted	FOnConnectionStarted;
	TClientDeviceFound			FOnDeviceFound;
	TClientValueChanged			FOnValueChanged;
	#pragma end_region Events

	#pragma region Helper method
	void __fastcall SetOldClient(TGattClient* Client);
	void __fastcall RemoveClient(TGattClient* Client);
	TGattClient* __fastcall FindClient(const __int64 Address);
	#pragma end_region Helper method

	#pragma region Client event handlers
	void __fastcall ClientDisconnect(TObject* Sender, const int Reason);
	void __fastcall ClientConnect(TObject* Sender, const int Error);
	void __fastcall ClientCharacteristicChanged(TObject* Sender,
		const unsigned short Handle, const TwclGattCharacteristicValue Value);
	#pragma end_region Client event handlers

	#pragma region Events management
	void __fastcall DoClientDisconnected(const __int64 Address,
		const int Reason);
	void __fastcall DoConnectionCompleted(const __int64 Address,
		const int Result);
	void __fastcall DoConnectionStarted(const __int64 Address,
		const int Result);
	void __fastcall DoDeviceFound(const __int64 Address,
		const System::UnicodeString Name);
	void __fastcall DoValueChanged(const __int64 Address,
		const TwclGattCharacteristicValue Value);
	#pragma end_region Events management

protected:
	#pragma region Device search handling
	virtual void __fastcall DoAdvertisementFrameInformation(
		const __int64 Address, const __int64 Timestamp, const signed char Rssi,
		const System::UnicodeString Name,
		const TwclBluetoothLeAdvertisementType PacketType,
		const TwclBluetoothLeAdvertisementFlags Flags);
	virtual void __fastcall DoStopped();
	#pragma end_region Device search handling

public:
	#pragma region Constructor and destructor
	virtual __fastcall TClientWatcher();
	virtual __fastcall ~TClientWatcher();
	#pragma end_region Constructor and destructor

	#pragma region Communication methods
	int __fastcall Disconnect(const __int64 Address);
	int __fastcall ReadData(const __int64 Address,
		TwclGattCharacteristicValue& Data);
	int __fastcall WriteData(const __int64 Address,
		const TwclGattCharacteristicValue& Data);
	#pragma end_region Communication methods

	#pragma region Events
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
	#pragma enregion Events
};

#endif
