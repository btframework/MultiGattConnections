//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "main.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "wclBluetooth"
#pragma resource "*.dfm"
TfmMain *fmMain;
//---------------------------------------------------------------------------
__fastcall TfmMain::TfmMain(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acDisconnectExecute(TObject *Sender)
{
	if (lvDevices->Selected != NULL)
	{
		TListItem* Item = lvDevices->Selected;
		__int64 Address = StrToInt64("$" + Item->Caption);
		int Res = FWatcher->Disconnect(Address);
		if (Res != WCL_E_SUCCESS)
			ShowMessage("Disconnect failed: 0x" + IntToHex(Res, 8));
  	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acDisconnectUpdate(TObject *Sender)
{
	TListItem* Item = lvDevices->Selected;
	TDeviceStatus Status = dsFound;
	if (Item != NULL)
		Status = (TDeviceStatus)Item->Data;
	((TAction*)Sender)->Enabled = (Status == dsConnected);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acReadExecute(TObject *Sender)
{
	if (lvDevices->Selected != NULL)
	{
		TListItem* Item = lvDevices->Selected;
		__int64 Address = StrToInt64("$" + Item->Caption);
		TwclGattCharacteristicValue Data;
		int Res = FWatcher->ReadData(Address, Data);
		if (Res != WCL_E_SUCCESS)
			ShowMessage("Read failed: 0x" + IntToHex(Res, 8));
		else
		{
			if (Data.Length == 0)
				ShowMessage("Data is empty");
			else
			{
				System::AnsiString s;
				for (int i = 0; i < Data.Length; i++)
					s += (System::AnsiChar)Data[i];
				ShowMessage("Data read: " + s);
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acReadUpdate(TObject *Sender)
{
	TListItem* Item = lvDevices->Selected;
	TDeviceStatus Status = dsFound;
	if (Item != NULL)
		Status = (TDeviceStatus)Item->Data;

	((TAction*)Sender)->Enabled = (Status == dsConnected);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acStartExecute(TObject *Sender)
{
	int Res = Manager->Open();
	if (Res != WCL_E_SUCCESS)
		ShowMessage("Open Bluetooth Manager failed: 0x" + IntToHex(Res, 8));
	else
	{
		TwclBluetoothRadio* Radio;
		Res = Manager->GetLeRadio(Radio);
		if (Res != WCL_E_SUCCESS)
			ShowMessage("Get working radio failed: 0x" + IntToHex(Res, 8));
		else
		{
			Res = FWatcher->Start(Radio);
			if (Res != WCL_E_SUCCESS)
				ShowMessage("Start Watcher failed: 0x" + IntToHex(Res, 8));
		}

		if (Res != WCL_E_SUCCESS)
			Manager->Close();
  	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acStartUpdate(TObject *Sender)
{
  ((TAction*)Sender)->Enabled = !FWatcher->Monitoring;
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acStopExecute(TObject *Sender)
{
  FWatcher->Stop();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acStopUpdate(TObject *Sender)
{
	((TAction*)Sender)->Enabled = FWatcher->Monitoring;
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::acWriteExecute(TObject *Sender)
{
	if (lvDevices->Selected != NULL)
	{
		TListItem* Item = lvDevices->Selected;
		__int64 Address = StrToInt64("$" + Item->Caption);
		System::AnsiString s = edData->Text;
		TwclGattCharacteristicValue Data;
		Data.Length = s.Length();
		for (int i = 0; i < s.Length(); i++)
			Data[i] = (BYTE)s[i + 1];
		int Res = FWatcher->WriteData(Address, Data);
		if (Res != WCL_E_SUCCESS)
			ShowMessage("Write failed: 0x" + IntToHex(Res, 8));
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::acWriteUpdate(TObject *Sender)
{
	TListItem* Item = lvDevices->Selected;
	TDeviceStatus Status = dsFound;
	if (Item != NULL)
		Status = (TDeviceStatus)Item->Data;

	((TAction*)Sender)->Enabled = (Status == dsConnected && edData->Text != "");
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::btClearClick(TObject *Sender)
{
	lbLog->Items->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TfmMain::FormCreate(TObject *Sender)
{
	// Do not forget to change synchronization method.
	TwclMessageBroadcaster::SetSyncMethod(skThread);

	FWatcher = new TClientWatcher();
	FWatcher->OnClientDisconnected = WatcherClientDisconnected;
	FWatcher->OnConnectionCompleted = WatcherConnectionCompleted;
	FWatcher->OnConnectionStarted = WatcherConnectionStarted;
	FWatcher->OnDeviceFound = WatcherDeviceFound;
	FWatcher->OnValueChanged = WatcherValueChanged;
	FWatcher->OnStarted = WatcherStarted;
	FWatcher->OnStopped = WatcherStopped;

	InitializeCriticalSection(&FSyncCS);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormDestroy(TObject *Sender)
{
	acStop->Execute();

	FWatcher->Free();

	DeleteCriticalSection(&FSyncCS);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerAfterOpenSync()
{
	lbLog->Items->Add("Bluetooth Manager opened");
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerAfterOpen(TObject *Sender)
{
	if (MainThreadID == GetCurrentThreadId())
		ManagerAfterOpenSync();
	else
		TThread::Synchronize(NULL, ManagerAfterOpenSync);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerBeforeCloseSync()
{
	lbLog->Items->Add("Bluetooth Manager is closing");
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerBeforeClose(TObject *Sender)
{
	if (MainThreadID == GetCurrentThreadId())
		ManagerBeforeCloseSync();
	else
		TThread::Synchronize(NULL, ManagerBeforeCloseSync);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerClosedSync()
{
	lbLog->Items->Add("Bluetooth Manager closed");
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerClosed(TObject *Sender)
{
	if (MainThreadID == GetCurrentThreadId())
		ManagerClosedSync();
	else
		TThread::Synchronize(NULL, ManagerClosedSync);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherClientDisconnectedSync()
{
	lbLog->Items->Add("Device " + IntToHex(FSyncAddress, 12) +
		" disconnected: 0x" + IntToHex(FSyncResult, 8));

	if (lvDevices->Items->Count > 0)
	{
		for (int i = 0; i < lvDevices->Items->Count; i++)
		{
			TListItem* Item = lvDevices->Items->Item[i];
			__int64 DeviceAddress = StrToInt64("$" + Item->Caption);
			if (DeviceAddress == FSyncAddress)
			{
				lvDevices->Items->Delete(i);
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherClientDisconnected(const __int64 Address,
	const int Reason)
{
	EnterCriticalSection(&FSyncCS);
	__try
	{
		FSyncAddress = Address;
		FSyncResult = Reason;

		if (MainThreadID == GetCurrentThreadId())
			WatcherClientDisconnectedSync();
		else
			TThread::Synchronize(NULL, WatcherClientDisconnectedSync);
	}
	__finally
	{
		LeaveCriticalSection(&FSyncCS);
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherConnectionCompletedSync()
{
	lbLog->Items->Add("Connection to " + IntToHex(FSyncAddress, 12) +
		" completed: 0x" + IntToHex(FSyncResult, 8));

	if (lvDevices->Items->Count > 0)
	{
		for (int i = 0; i < lvDevices->Items->Count; i++)
		{
			TListItem* Item = lvDevices->Items->Item[i];
			__int64 DeviceAddress = StrToInt64("$" + Item->Caption);
			if (DeviceAddress == FSyncAddress)
			{
				if (FSyncResult != WCL_E_SUCCESS)
					lvDevices->Items->Delete(i);
				else
				{
					Item->SubItems->Strings[1] = "Connected";
					Item->Data = (void*)dsConnected;
				}
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherConnectionCompleted(const __int64 Address,
	const int Result)
{
	EnterCriticalSection(&FSyncCS);
	__try
	{
		FSyncAddress = Address;
		FSyncResult = Result;

		if (MainThreadID == GetCurrentThreadId())
			WatcherConnectionCompletedSync();
		else
			TThread::Synchronize(NULL, WatcherConnectionCompletedSync);
	}
	__finally
	{
		LeaveCriticalSection(&FSyncCS);
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherConnectionStartedSync()
{
	lbLog->Items->Add("Connection to " + IntToHex(FSyncAddress, 12) +
		" started: 0x" + IntToHex(FSyncResult, 8));

	if (lvDevices->Items->Count > 0)
	{
		for (int i = 0; i < lvDevices->Items->Count; i++)
		{
			TListItem* Item = lvDevices->Items->Item[i];
			__int64 DeviceAddress = StrToInt64("$" + Item->Caption);
			if (DeviceAddress == FSyncAddress)
			{
				if (FSyncResult != WCL_E_SUCCESS)
					lvDevices->Items->Delete(i);
				else
				{
					Item->SubItems->Strings[1] = "Connecting...";
					Item->Data = (void*)dsConnecting;
				}
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherConnectionStarted(const __int64 Address,
	const int Result)
{
	EnterCriticalSection(&FSyncCS);
	__try
	{
		FSyncAddress = Address;
		FSyncResult = Result;

		if (MainThreadID == GetCurrentThreadId())
			WatcherConnectionStartedSync();
		else
			TThread::Synchronize(NULL, WatcherConnectionStartedSync);
	}
	__finally
	{
		LeaveCriticalSection(&FSyncCS);
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherDeviceFoundSync()
{
	lbLog->Items->Add("Device " + IntToHex(FSyncAddress, 12) + " found: " +
		FSyncName);

	TListItem* Item = lvDevices->Items->Add();
	Item->Caption = IntToHex(FSyncAddress, 12);
	Item->SubItems->Add(FSyncName);
	Item->SubItems->Add("Found...");
	Item->Data = (void*)dsFound;
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherDeviceFound(const __int64 Address,
	const System::UnicodeString Name)
{
	EnterCriticalSection(&FSyncCS);
	__try
	{
		FSyncAddress = Address;
		FSyncName = Name;

		if (MainThreadID == GetCurrentThreadId())
			WatcherDeviceFoundSync();
		else
			TThread::Synchronize(NULL, WatcherDeviceFoundSync);
	}
	__finally
	{
		LeaveCriticalSection(&FSyncCS);
	}
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherStoppedSync()
{
	lvDevices->Items->Clear();
	lbLog->Items->Add("Client watcher stopped");

	Manager->Close();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherStopped(TObject* Sender)
{
	if (MainThreadID == GetCurrentThreadId())
		WatcherStoppedSync();
	else
		TThread::Synchronize(NULL, WatcherStoppedSync);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherStartedSync()
{
	lvDevices->Items->Clear();
	lbLog->Items->Add("Client watcher started");
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherStarted(TObject* Sender)
{
	if (MainThreadID == GetCurrentThreadId())
		WatcherStartedSync();
	else
		TThread::Synchronize(NULL, WatcherStartedSync);
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherValueChangedSync()
{
	if (FSyncValue.Length > 3)
	{
		unsigned long Val = (FSyncValue[3] << 24) | (FSyncValue[2] << 16) |
			(FSyncValue[1] << 8) | FSyncValue[0];
		lbLog->Items->Add("Data received from " + IntToHex(FSyncAddress, 12) +
			": " + IntToStr((int)Val));
	}
	else
		lbLog->Items->Add("Empty data received");
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherValueChanged(const __int64 Address,
	const TwclGattCharacteristicValue& Value)
{
	EnterCriticalSection(&FSyncCS);
	__try
	{
		FSyncAddress = Address;
		FSyncValue = Value;

		if (MainThreadID == GetCurrentThreadId())
			WatcherValueChangedSync();
		else
			TThread::Synchronize(NULL, WatcherValueChangedSync);
	}
	__finally
	{
		LeaveCriticalSection(&FSyncCS);
	}
}
//---------------------------------------------------------------------------
