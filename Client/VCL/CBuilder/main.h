//---------------------------------------------------------------------------

#ifndef mainH
#define mainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include "ClientWatcher.h"
#include "wclBluetooth.hpp"
#include <System.Actions.hpp>
#include <Vcl.ActnList.hpp>
#include <Vcl.ComCtrls.hpp>
//---------------------------------------------------------------------------
class TfmMain : public TForm
{
__published:	// IDE-managed Components
	TLabel *laData;
	TButton *btStart;
	TButton *btStop;
	TListView *lvDevices;
	TButton *btDisconnect;
	TEdit *edData;
	TButton *btRead;
	TButton *btWrite;
	TListBox *lbLog;
	TButton *btClear;
	TActionList *ActionList;
	TAction *acStart;
	TAction *acStop;
	TAction *acDisconnect;
	TAction *acRead;
	TAction *acWrite;
	TwclBluetoothManager *Manager;
	void __fastcall acDisconnectExecute(TObject *Sender);
	void __fastcall acDisconnectUpdate(TObject *Sender);
	void __fastcall acReadExecute(TObject *Sender);
	void __fastcall acReadUpdate(TObject *Sender);
	void __fastcall acStartExecute(TObject *Sender);
	void __fastcall acStartUpdate(TObject *Sender);
	void __fastcall acStopExecute(TObject *Sender);
	void __fastcall acStopUpdate(TObject *Sender);
	void __fastcall acWriteExecute(TObject *Sender);
	void __fastcall acWriteUpdate(TObject *Sender);
	void __fastcall btClearClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall ManagerAfterOpen(TObject *Sender);
	void __fastcall ManagerBeforeClose(TObject *Sender);
	void __fastcall ManagerClosed(TObject *Sender);

private:	// User declarations
	typedef enum {
		dsFound,
		dsConnecting,
		dsConnected
	} TDeviceStatus;

	TClientWatcher*	FWatcher;

	void __fastcall WatcherStopped(TObject* Sender);
	void __fastcall WatcherStarted(TObject* Sender);
	void __fastcall WatcherValueChanged(const __int64 Address,
		const TwclGattCharacteristicValue& Value);
	void __fastcall WatcherDeviceFound(const __int64 Address,
		const System::UnicodeString Name);
	void __fastcall WatcherConnectionStarted(const __int64 Address,
		const int Result);
	void __fastcall WatcherConnectionCompleted(const __int64 Address,
		const int Result);
	void __fastcall WatcherClientDisconnected(const __int64 Address,
		const int Reason);

public:		// User declarations
	__fastcall TfmMain(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfmMain *fmMain;
//---------------------------------------------------------------------------
#endif
