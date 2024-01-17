#pragma once

#include <list>

#include "wclBluetooth.h"
#include "GattClient.h"

using namespace std;
using namespace wclCommon;
using namespace wclCommunication;
using namespace wclBluetooth;

#define ClientConnectionCompleted(_event_name_) \
	__event void _event_name_(const __int64 Address, const int Error)
#define ClientConnectionStarted(_event_name_) \
	__event void _event_name_(const __int64 Address, const int Result)
#define ClientDeviceFound(_event_name_) \
	__event void _event_name_(const __int64 Address, const tstring& name)
#define ClientDisconnected(_event_name_) \
	__event void _event_name_(const __int64 Address, const int Reason)
#define ClientValueChanged(_event_name_) \
	__event void _event_name_(const __int64 Address, const unsigned char* Value, \
	const unsigned long Length);

const tstring DEVICE_NAME = _T("MultyGattServer");

class CClientWatcher : public CwclBluetoothLeBeaconWatcher
{
	DISABLE_COPY(CClientWatcher);

private:
#pragma region Connections management
	RTL_CRITICAL_SECTION	FConnectionsCS;
	list<CGattClient*>*		FConnections;
	list<__int64>*			FPendingConnections;
	CGattClient*			FOldClient;
#pragma endregion Connections management

#pragma region Helper method
	void __fastcall SetOldClient(CGattClient* Client);
	void __fastcall RemoveClient(CGattClient* Client);
	CGattClient* __fastcall FindClient(const __int64 Address);
	void CopyClients(list<CGattClient*>* Clients);
	void CreateClient(const __int64 Address);
#pragma endregion Helper method

#pragma region Client event handlers
	void ClientDisconnect(void* Sender, const int Reason);
	void ClientConnect(void* Sender, const int Error);
	void ClientCharacteristicChanged(void* Sender, const unsigned short Handle,
		const unsigned char* Value, const unsigned long Length);
#pragma endregion Client event handlers

#pragma region Events management
	void DoClientDisconnected(const __int64 Address, const int Reason);
	void DoConnectionCompleted(const __int64 Address, const int Result);
	void DoConnectionStarted(const __int64 Address, const int Result);
	void DoDeviceFound(const __int64 Address, const tstring& Name);
	void DoValueChanged(const __int64 Address, const unsigned char* Value,
		const unsigned long Length);
#pragma endregion Events management

protected:
#pragma region Device search handling
	virtual void DoAdvertisementFrameInformation(const __int64 Address,
		const __int64 Timestamp, const char Rssi, const tstring& Name,
		const wclBluetoothLeAdvertisementType PacketType,
		const wclBluetoothLeAdvertisementFlags& Flags) override;
	virtual void DoStopped() override;
#pragma endregion Device search handling

public:
#pragma region Constructor and destructor
	CClientWatcher();
	virtual ~CClientWatcher();
#pragma endregion Constructor and destructor

#pragma region Communication methods
	int Disconnect(const __int64 Address);
	int ReadData(const __int64 Address, unsigned char*& Data,
		unsigned long& Length);
	int WriteData(const __int64 Address, const unsigned char* const Data,
		const unsigned long Length);
#pragma endregion Communication methods

#pragma region Events
	ClientDisconnected(OnClientDisconnected);
	ClientConnectionCompleted(OnConnectionCompleted);
	ClientConnectionStarted(OnConnectionStarted);
	ClientDeviceFound(OnDeviceFound);
	ClientValueChanged(OnValueChanged);
#pragma endregion Events
};
