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
		Status = (TDeviceStatus)((int)Item->Data);
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
		Status = (TDeviceStatus)((int)Item->Data);

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
		Status = (TDeviceStatus)((int)Item->Data);

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
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::FormDestroy(TObject *Sender)
{
	acStop->Execute();

	FWatcher->Free();
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerAfterOpen(TObject *Sender)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this](){
		lbLog->Items->Add("Bluetooth Manager opened");
	}));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerBeforeClose(TObject *Sender)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this](){
		lbLog->Items->Add("Bluetooth Manager is closing");
	}));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::ManagerClosed(TObject *Sender)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this](){
		lbLog->Items->Add("Bluetooth Manager closed");
	}));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherClientDisconnected(const __int64 Address,
	const int Reason)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this, Address, Reason](){
		lbLog->Items->Add("Device " + IntToHex(Address, 12) +
			" disconnected: 0x" + IntToHex(Reason, 8));
		if (lvDevices->Items->Count > 0)
		{
			for (int i = 0; i < lvDevices->Items->Count; i++)
			{
				TListItem* Item = lvDevices->Items->Item[i];
				__int64 DeviceAddress = StrToInt64("$" + Item->Caption);
				if (DeviceAddress == Address)
				{
					lvDevices->Items->Delete(i);
					break;
				}
			}
		}
	}));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherConnectionCompleted(const __int64 Address,
	const int Result)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this, Address, Result](){
		lbLog->Items->Add("Connection to " + IntToHex(Address, 12) +
			" completed: 0x" + IntToHex(Result, 8));
		if (lvDevices->Items->Count > 0)
		{
			for (int i = 0; i < lvDevices->Items->Count; i++)
			{
				TListItem* Item = lvDevices->Items->Item[i];
				__int64 DeviceAddress = StrToInt64("$" + Item->Caption);
				if (DeviceAddress == Address)
				{
					if (Result != WCL_E_SUCCESS)
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
    }));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherConnectionStarted(const __int64 Address,
	const int Result)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this, Address, Result](){
		lbLog->Items->Add("Connection to " + IntToHex(Address, 12) +
			" started: 0x" + IntToHex(Result, 8));
		if (lvDevices->Items->Count > 0)
		{
			for (int i = 0; i < lvDevices->Items->Count; i++)
			{
				TListItem* Item = lvDevices->Items->Item[i];
				__int64 DeviceAddress = StrToInt64("$" + Item->Caption);
				if (DeviceAddress == Address)
				{
					if (Result != WCL_E_SUCCESS)
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
	}));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherDeviceFound(const __int64 Address,
	const System::UnicodeString Name)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this, Address, Name](){
		lbLog->Items->Add("Device " + IntToHex(Address, 12) + " found: " +
			Name);
		TListItem* Item = lvDevices->Items->Add();
		Item->Caption = IntToHex(Address, 12);
		Item->SubItems->Add(Name);
		Item->SubItems->Add("Found...");
		Item->Data = (void*)dsFound;
	}));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherStopped(TObject* Sender)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this](){
		lvDevices->Items->Clear();
		lbLog->Items->Add("Client watcher stopped");
		Manager->Close();
	}));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherStarted(TObject* Sender)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this](){
		lvDevices->Items->Clear();
		lbLog->Items->Add("Client watcher started");
	}));
}
//---------------------------------------------------------------------------
void __fastcall TfmMain::WatcherValueChanged(const __int64 Address,
	const TwclGattCharacteristicValue& Value)
{
	TThread::Synchronize(NULL, _di_TThreadProcedure([this, Address, Value](){
		if (Value.Length > 3)
		{
			unsigned long Val = (Value[3] << 24) | (Value[2] << 16) |
			  (Value[1] << 8) | Value[0];
			lbLog->Items->Add("Data received from " + IntToHex(Address, 12) +
				": " + IntToStr((int)Val));
		}
		else
			lbLog->Items->Add("Empty data received");
    }));
}
//---------------------------------------------------------------------------
