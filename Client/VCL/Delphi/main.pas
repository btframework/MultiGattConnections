unit main;

interface

uses
  Vcl.Forms, Vcl.Controls, Vcl.ComCtrls, System.Classes, System.Actions,
  Vcl.ActnList, Vcl.StdCtrls, ClientWatcher, wclBluetooth, Windows;

type
  TfmMain = class(TForm)
    btStart: TButton;
    ActionList: TActionList;
    acStart: TAction;
    acStop: TAction;
    acDisconnect: TAction;
    acRead: TAction;
    acWrite: TAction;
    btStop: TButton;
    lvDevices: TListView;
    btDisconnect: TButton;
    laData: TLabel;
    edData: TEdit;
    btRead: TButton;
    btWrite: TButton;
    lbLog: TListBox;
    btClear: TButton;
    Manager: TwclBluetoothManager;

    {$REGION Bluetooth Manager event handlers}
    procedure ManagerClosed(Sender: TObject);
    procedure ManagerBeforeClose(Sender: TObject);
    procedure ManagerAfterOpen(Sender: TObject);
    {$ENDREGION Form event handlers}

    {$REGION Form event handlers}
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    {$ENDREGION Form event handlers}

    {$REGION UI update}
    procedure acStartUpdate(Sender: TObject);
    procedure acStopUpdate(Sender: TObject);
    procedure acDisconnectUpdate(Sender: TObject);
    procedure acReadUpdate(Sender: TObject);
    procedure acWriteUpdate(Sender: TObject);
    {$ENDREGION UI update}

    {$REGION Buttons event handlers}
    procedure acStartExecute(Sender: TObject);
    procedure acStopExecute(Sender: TObject);
    procedure acDisconnectExecute(Sender: TObject);
    procedure acReadExecute(Sender: TObject);
    procedure acWriteExecute(Sender: TObject);
    procedure btClearClick(Sender: TObject);
    {$ENDREGION Buttons event handlers}

  private type
    TDeviceStatus = (
      dsFound,
      dsConnecting,
      dsConnected
    );

  private
    FWatcher: TClientWatcher;

    {$REGION Synchronized members}
    FSyncCS: RTL_CRITICAL_SECTION;
    FSyncAddress: Int64;
    FSyncResult: Integer;
    FSyncName: string;
    FSyncValue: TwclGattCharacteristicValue;
    {$ENDREGION Synchronized members}

    {$REGION Client Watcher event handlers}
    procedure WatcherStopped(Sender: TObject);
    procedure WatcherStarted(Sender: TObject);
    procedure WatcherValueChanged(const Address: Int64;
      const Value: TwclGattCharacteristicValue);
    procedure WatcherDeviceFound(const Address: Int64; const Name: string);
    procedure WatcherConnectionStarted(const Address: Int64;
      const Result: Integer);
    procedure WatcherConnectionCompleted(const Address: Int64;
      const Result: Integer);
    procedure WatcherClientDisconnected(const Address: Int64;
      const Reason: Integer);
    {$ENDREGION Client Watcher event handlers}

    {$REGION Synchronized methods}
    procedure ManagerAfterOpenSync;
    procedure ManagerBeforeCloseSync;
    procedure ManagerClosedSync;

    procedure WatcherStoppedSync;
    procedure WatcherStartedSync;
    procedure WatcherValueChangedSync;
    procedure WatcherDeviceFoundSync;
    procedure WatcherConnectionStartedSync;
    procedure WatcherConnectionCompletedSync;
    procedure WatcherClientDisconnectedSync;
    {$ENDREGION Synchronized methods}
  end;

var
  fmMain: TfmMain;

implementation

uses
  SysUtils, wclErrors, wclMessaging, Dialogs;

{$R *.dfm}

procedure TfmMain.acDisconnectExecute(Sender: TObject);
var
  Item: TListItem;
  Address: Int64;
  Res: Integer;
begin
  if lvDevices.Selected <> nil then begin
    Item := lvDevices.Selected;
    Address := StrToInt64('$' + Item.Caption);
    Res := FWatcher.Disconnect(Address);
    if Res <> WCL_E_SUCCESS then
      ShowMessage('Disconnect failed: 0x' + IntToHex(Res, 8));
  end;
end;

procedure TfmMain.acDisconnectUpdate(Sender: TObject);
var
  Item: TListItem;
  Status: TDeviceStatus;
begin
  Item := lvDevices.Selected;
  if Item <> nil then
    Status := TDeviceStatus(Item.Data)
  else
    Status := dsFound;

  TAction(Sender).Enabled := Status = dsConnected;
end;

procedure TfmMain.acReadExecute(Sender: TObject);
var
  Item: TListItem;
  Address: Int64;
  Data: TwclGattCharacteristicValue;
  Res: Integer;
  s: AnsiString;
begin
  if lvDevices.Selected <> nil then begin
    Item := lvDevices.Selected;
    Address := StrToInt64('$' + Item.Caption);
    Res := FWatcher.ReadData(Address, Data);
    if Res <> WCL_E_SUCCESS then
      ShowMessage('Read failed: 0x' + IntToHex(Res, 8))

    else begin
      if Length(Data) = 0 then
        ShowMessage('Data is empty')

      else begin
        SetLength(s, Length(Data));
        CopyMemory(Pointer(s), Pointer(Data), Length(Data));
        ShowMessage('Data read: ' + String(s));
      end;
    end;
  end;
end;

procedure TfmMain.acReadUpdate(Sender: TObject);
var
  Item: TListItem;
  Status: TDeviceStatus;
begin
  Item := lvDevices.Selected;
  if Item <> nil then
    Status := TDeviceStatus(Item.Data)
  else
    Status := dsFound;

  TAction(Sender).Enabled := Status = dsConnected;
end;

procedure TfmMain.acStartExecute(Sender: TObject);
var
  Res: Integer;
  Radio: TwclBluetoothRadio;
begin
  Res := Manager.Open;
  if Res <> WCL_E_SUCCESS then
    ShowMessage('Open Bluetooth Manager failed: 0x' + IntToHex(Res, 8))

  else begin
    Res := Manager.GetLeRadio(Radio);
    if Res <> WCL_E_SUCCESS then
      ShowMessage('Get working radio failed: 0x' + IntToHex(Res, 8))

    else begin
      Res := FWatcher.Start(Radio);
      if Res <> WCL_E_SUCCESS then
        ShowMessage('Start Watcher failed: 0x' + IntToHex(Res, 8));
    end;

    if Res <> WCL_E_SUCCESS then
      Manager.Close;
  end;
end;

procedure TfmMain.acStartUpdate(Sender: TObject);
begin
  TAction(Sender).Enabled := not FWatcher.Monitoring;
end;

procedure TfmMain.acStopExecute(Sender: TObject);
begin
  FWatcher.Stop;
end;

procedure TfmMain.acStopUpdate(Sender: TObject);
begin
  TAction(Sender).Enabled := FWatcher.Monitoring;
end;

procedure TfmMain.acWriteExecute(Sender: TObject);
var
  Item: TListItem;
  Address: Int64;
  Data: TwclGattCharacteristicValue;
  Res: Integer;
  s: AnsiString;
begin
  if lvDevices.Selected <> nil then begin
    Item := lvDevices.Selected;
    Address := StrToInt64('$' + Item.Caption);
    s := AnsiString(edData.Text);
    SetLength(Data, Length(s));
    CopyMemory(Pointer(Data), Pointer(s), Length(s));
    Res := FWatcher.WriteData(Address, Data);
    if Res <> WCL_E_SUCCESS then
      ShowMessage('Write failed: 0x' + IntToHex(Res, 8));
  end;
end;

procedure TfmMain.acWriteUpdate(Sender: TObject);
var
  Item: TListItem;
  Status: TDeviceStatus;
begin
  Item := lvDevices.Selected;
  if Item <> nil then
    Status := TDeviceStatus(Item.Data)
  else
    Status := dsFound;

  TAction(Sender).Enabled := (Status = dsConnected) and (edData.Text <> '');
end;

procedure TfmMain.btClearClick(Sender: TObject);
begin
  lbLog.Items.Clear;
end;

procedure TfmMain.FormCreate(Sender: TObject);
begin
  // Do not forget to change synchronization method.
  TwclMessageBroadcaster.SetSyncMethod(skThread);

  FWatcher := TClientWatcher.Create;
  FWatcher.OnClientDisconnected := WatcherClientDisconnected;
  FWatcher.OnConnectionCompleted := WatcherConnectionCompleted;
  FWatcher.OnConnectionStarted := WatcherConnectionStarted;
  FWatcher.OnDeviceFound := WatcherDeviceFound;
  FWatcher.OnValueChanged := WatcherValueChanged;
  FWatcher.OnStarted := WatcherStarted;
  FWatcher.OnStopped := WatcherStopped;

  InitializeCriticalSection(FSyncCS);
end;

procedure TfmMain.FormDestroy(Sender: TObject);
begin
  acStop.Execute;

  FWatcher.Free;

  DeleteCriticalSection(FSyncCS);
end;

procedure TfmMain.ManagerAfterOpenSync;
begin
  lbLog.Items.Add('Bluetooth Manager opened');
end;

procedure TfmMain.ManagerAfterOpen(Sender: TObject);
begin
  if MainThreadID = GetCurrentThreadId then
    ManagerAfterOpenSync
  else
    TThread.Synchronize(nil, ManagerAfterOpenSync);
end;

procedure TfmMain.ManagerBeforeCloseSync;
begin
  lbLog.Items.Add('Bluetooth Manager is closing');
end;

procedure TfmMain.ManagerBeforeClose(Sender: TObject);
begin
  if MainThreadID = GetCurrentThreadId then
    ManagerBeforeCloseSync
  else
    TThread.Synchronize(nil, ManagerBeforeCloseSync);
end;

procedure TfmMain.ManagerClosedSync;
begin
  lbLog.Items.Add('Bluetooth Manager closed');
end;

procedure TfmMain.ManagerClosed(Sender: TObject);
begin
  if MainThreadID = GetCurrentThreadId then
    ManagerClosedSync
  else
    TThread.Synchronize(nil, ManagerClosedSync);
end;

procedure TfmMain.WatcherClientDisconnectedSync;
var
  Item: TListItem;
  DeviceAddress: Int64;
begin
  lbLog.Items.Add('Device ' + IntToHex(FSyncAddress, 12) + ' disconnected: 0x' +
    IntToHex(FSyncResult, 8));

  if lvDevices.Items.Count > 0 then begin
    for Item in lvDevices.Items do begin
      DeviceAddress := StrToInt64('$' + Item.Caption);
      if DeviceAddress = FSyncAddress then begin
        lvDevices.Items.Delete(Item.Index);
        Break;
      end;
    end;
  end;
end;

procedure TfmMain.WatcherClientDisconnected(const Address: Int64;
  const Reason: Integer);
begin
  EnterCriticalSection(FSyncCS);
  try
    FSyncAddress := Address;
    FSyncResult := Reason;

    if MainThreadID = GetCurrentThreadId then
      WatcherClientDisconnectedSync
    else
      TThread.Synchronize(nil, WatcherClientDisconnectedSync);
  finally
    LeaveCriticalSection(FSyncCS);
  end;
end;

procedure TfmMain.WatcherConnectionCompletedSync;
var
  Item: TListItem;
  DeviceAddress: Int64;
begin
  lbLog.Items.Add('Connection to ' + IntToHex(FSyncAddress, 12) +
    ' completed: 0x' + IntToHex(FSyncResult, 8));

  if lvDevices.Items.Count > 0 then begin
    for Item in lvDevices.Items do begin
      DeviceAddress := StrToInt64('$' + Item.Caption);
      if DeviceAddress = FSyncAddress then begin
        if FSyncResult <> WCL_E_SUCCESS then
          lvDevices.Items.Delete(Item.Index)
        else begin
          Item.SubItems[1] := 'Connected';
          Item.Data := Pointer(dsConnected);
        end;
        Break;
      end;
    end;
  end;
end;

procedure TfmMain.WatcherConnectionCompleted(const Address: Int64;
  const Result: Integer);
begin
  EnterCriticalSection(FSyncCS);
  try
    FSyncAddress := Address;
    FSyncResult := Result;

    if MainThreadID = GetCurrentThreadId then
      WatcherConnectionCompletedSync
    else
      TThread.Synchronize(nil, WatcherConnectionCompletedSync);
  finally
    LeaveCriticalSection(FSyncCS);
  end;
end;

procedure TfmMain.WatcherConnectionStartedSync;
var
  Item: TListItem;
  DeviceAddress: Int64;
begin
  lbLog.Items.Add('Connection to ' + IntToHex(FSyncAddress, 12) +
    ' started: 0x' + IntToHex(FSyncResult, 8));

  if lvDevices.Items.Count > 0 then begin
    for Item in lvDevices.Items do begin
      DeviceAddress := StrToInt64('$' + Item.Caption);
      if DeviceAddress = FSyncAddress then begin
        if FSyncResult <> WCL_E_SUCCESS then
          lvDevices.Items.Delete(Item.Index)
        else begin
          Item.SubItems[1] := 'Connecting...';
          Item.Data := Pointer(dsConnecting);
        end;
        Break;
      end;
    end;
  end;
end;

procedure TfmMain.WatcherConnectionStarted(const Address: Int64;
  const Result: Integer);
begin
  EnterCriticalSection(FSyncCS);
  try
    FSyncAddress := Address;
    FSyncResult := Result;

    if MainThreadID = GetCurrentThreadId then
      WatcherConnectionStartedSync
    else
      TThread.Synchronize(nil, WatcherConnectionStartedSync);
  finally
    LeaveCriticalSection(FSyncCS);
  end;
end;

procedure TfmMain.WatcherDeviceFoundSync;
var
  Item: TListItem;
begin
  lbLog.Items.Add('Device ' + IntToHex(FSyncAddress, 12) + ' found: ' +
    FSyncName);

  Item := lvDevices.Items.Add();
  Item.Caption := IntToHex(FSyncAddress, 12);
  Item.SubItems.Add(FSyncName);
  Item.SubItems.Add('Found...');
  Item.Data := Pointer(dsFound);
end;

procedure TfmMain.WatcherDeviceFound(const Address: Int64; const Name: string);
begin
  EnterCriticalSection(FSyncCS);
  try
    FSyncAddress := Address;
    FSyncName := Name;

    if MainThreadID = GetCurrentThreadId then
      WatcherDeviceFoundSync
    else
      TThread.Synchronize(nil, WatcherDeviceFoundSync);
  finally
    LeaveCriticalSection(FSyncCS);
  end;
end;

procedure TfmMain.WatcherStartedSync;
begin
  lvDevices.Items.Clear;
  lbLog.Items.Add('Client watcher started');
end;

procedure TfmMain.WatcherStarted(Sender: TObject);
begin
  if MainThreadID = GetCurrentThreadId then
    WatcherStartedSync
  else
    TThread.Synchronize(nil, WatcherStartedSync);
end;

procedure TfmMain.WatcherStoppedSync;
begin
  lvDevices.Items.Clear;
  lbLog.Items.Add('Client watcher stopped');

  Manager.Close;
end;

procedure TfmMain.WatcherStopped(Sender: TObject);
begin
  if MainThreadID = GetCurrentThreadId then
    WatcherStoppedSync
  else
    TThread.Synchronize(nil, WatcherStoppedSync);
end;

procedure TfmMain.WatcherValueChangedSync;
var
  Val: Cardinal;
begin
  if Length(FSyncValue) > 3 then begin
    Val := (FSyncValue[3] shl 24) or (FSyncValue[2] shl 16) or
      (FSyncValue[1] shl 8) or FSyncValue[0];
    lbLog.Items.Add('Data received from ' + IntToHex(FSyncAddress, 12) + ': ' +
      IntToStr(Val));
  end else
    lbLog.Items.Add('Empty data received');
end;

procedure TfmMain.WatcherValueChanged(const Address: Int64;
  const Value: TwclGattCharacteristicValue);
begin
  EnterCriticalSection(FSyncCS);
  try
    FSyncAddress := Address;
    FSyncValue := Value;

    if MainThreadID = GetCurrentThreadId then
      WatcherValueChangedSync
    else
      TThread.Synchronize(nil, WatcherValueChangedSync);
  finally
    LeaveCriticalSection(FSyncCS);
  end;
end;

end.
