unit ClientWatcher;

interface

uses
  wclBluetooth, Windows, System.Generics.Collections, GattClient;

type
  TClientConnectionCompleted = procedure(const Address: Int64;
    const Result: Integer) of object;
  TClientConnectionStarted = procedure(const Address: Int64;
    const Result: Integer) of object;
  TClientDeviceFound = procedure(const Address: Int64;
    const Name: string) of object;
  TClientDisconnected = procedure(const Address: Int64;
    const Reason: Integer) of object;
  TClientValueChanged = procedure(const Address: Int64;
    const Value: TwclGattCharacteristicValue) of object;

  TClientWatcher = class(TwclBluetoothLeBeaconWatcher)
  private const
    DEVICE_NAME = 'MultyGattServer';

  private
    {$Region Connections management}
    FConnectionsCS: RTL_CRITICAL_SECTION;
    FConnections: TList<TGattClient>;
    FPendingConnections: TList<Int64>;
    FOldClient: TGattClient;
    {$EndRegion Connections management}

    {$Region Events}
    FOnClientDisconnected: TClientDisconnected;
    FOnConnectionCompleted: TClientConnectionCompleted;
    FOnConnectionStarted: TClientConnectionStarted;
    FOnDeviceFound: TClientDeviceFound;
    FOnValueChanged: TClientValueChanged;
    {$EndRegion Events}

    {$Region Helper method}
    procedure SetOldClient(const Client: TGattClient);
    procedure RemoveClient(const Client: TGattClient);
    function FindClient(const Address: Int64): TGattClient;
    {$EndRegion Helper method}

    {$Region Client event handlers}
    procedure ClientDisconnect(Sender: TObject; const Reason: Integer);
    procedure ClientConnect(Sender: TObject; const Error: Integer);
    procedure ClientCharacteristicChanged(Sender: TObject; const Handle: Word;
      const Value: TwclGattCharacteristicValue);
    {$EndRegion Client event handlers}

    {$Region Events management}
    procedure DoClientDisconnected(const Address: Int64; const Reason: Integer);
    procedure DoConnectionCompleted(const Address: Int64;
      const Result: Integer);
    procedure DoConnectionStarted(const Address: Int64; const Result: Integer);
    procedure DoDeviceFound(const Address: Int64; const Name: string);
    procedure DoValueChanged(const Address: Int64;
      const Value: TwclGattCharacteristicValue);
    {$EndRegion Events management}

  protected
    {$Region Device search handling}
    procedure DoAdvertisementFrameInformation(const Address: Int64;
      const Timestamp: Int64; const Rssi: ShortInt; const Name: string;
      const PacketType: TwclBluetoothLeAdvertisementType;
      const Flags: TwclBluetoothLeAdvertisementFlags); override;
    procedure DoStopped; override;
    {$EndRegion Device search handling}

  public
    {$Region Constructor and destructor}
    constructor Create; reintroduce;
    destructor Destroy; override;
    {$EndRegion Constructor and destructor}

    {$Region Communication methods}
    function Disconnect(const Address: Int64): Integer;
    function ReadData(const Address: Int64;
      out Data: TwclGattCharacteristicValue): Integer;
    function WriteData(const Address: Int64;
      const Data: TwclGattCharacteristicValue): Integer;
    {$EndRegion Communication methods}

    {$Region Events}
    property OnClientDisconnected: TClientDisconnected
      read FOnClientDisconnected write FOnClientDisconnected;
    property OnConnectionCompleted: TClientConnectionCompleted
      read FOnConnectionCompleted write FOnConnectionCompleted;
    property OnConnectionStarted: TClientConnectionStarted
      read FOnConnectionStarted write FOnConnectionStarted;
    property OnDeviceFound: TClientDeviceFound read FOnDeviceFound
      write FOnDeviceFound;
    property OnValueChanged: TClientValueChanged read FOnValueChanged
      write FOnValueChanged;
    {$EndRegion Events}
  end;

implementation

uses
  wclErrors, wclBluetoothErrors, wclConnectionErrors;

{ TClientWatcher }

procedure TClientWatcher.ClientCharacteristicChanged(Sender: TObject;
  const Handle: Word; const Value: TwclGattCharacteristicValue);
var
  Client: TGattClient;
begin
  if Monitoring then begin
    Client := TGattClient(Sender);
    // Simple call the value changed event.
    DoValueChanged(Client.Address, Value);
  end;
end;

procedure TClientWatcher.ClientConnect(Sender: TObject; const Error: Integer);
var
  Client: TGattClient;
begin
  Client := TGattClient(Sender);

  // If we stopped we still can get client connection event.
  if not Monitoring then begin
    // Disconnect client and set the connection error.
    if Error = WCL_E_SUCCESS then
      Client.Disconnect
    else
      RemoveClient(Client);
    Exit;
  end;

  // If connection failed remove client from connections list.
  if Error <> WCL_E_SUCCESS then
    RemoveClient(Client)
  else begin
    // Othewrwise - add it to the connected clients list.
    EnterCriticalSection(FConnectionsCS);
    try
      FConnections.Add(Client);
    finally
      LeaveCriticalSection(FConnectionsCS);
    end;
  end;

  // Call connection completed event.
  DoConnectionCompleted(Client.Address, Error);
end;

procedure TClientWatcher.ClientDisconnect(Sender: TObject;
  const Reason: Integer);
var
  Client: TGattClient;
begin
  Client := TGattClient(Sender);
  // Call disconnect event.
  DoClientDisconnected(Client.Address, Reason);
  // Remove client from the lists.
  RemoveClient(Client);
end;

constructor TClientWatcher.Create;
begin
  inherited Create(nil);

  InitializeCriticalSection(FConnectionsCS);

  FConnections := TList<TGattClient>.Create;
  FPendingConnections := TList<Int64>.Create;
  FOldClient := nil;

  OnClientDisconnected := nil;
  OnConnectionCompleted := nil;
  OnConnectionStarted := nil;
  OnDeviceFound := nil;
  OnValueChanged := nil;
end;

destructor TClientWatcher.Destroy;
begin
  // We have to call stop here to prevent from issues with objects!
  Stop;

  FConnections.Free;
  FPendingConnections.Free;

  if FOldClient <> nil then
    FOldClient.Free;

  DeleteCriticalSection(FConnectionsCS);

  inherited;
end;

function TClientWatcher.Disconnect(const Address: Int64): Integer;
var
  Client: TGattClient;
begin
  if not Monitoring then
    Result := WCL_E_CONNECTION_CLOSED

  else begin
    EnterCriticalSection(FConnectionsCS);
    try
      Client := FindClient(Address);
      if Client = nil then
        Result := WCL_E_CONNECTION_NOT_ACTIVE
      else
        Result := Client.Disconnect;
    finally
      LeaveCriticalSection(FConnectionsCS);
    end;
  end;
end;

procedure TClientWatcher.DoAdvertisementFrameInformation(const Address: Int64;
  const Timestamp: Int64; const Rssi: ShortInt; const Name: string;
  const PacketType: TwclBluetoothLeAdvertisementType;
  const Flags: TwclBluetoothLeAdvertisementFlags);
var
  Client: TGattClient;
  Result: Integer;
begin
  // Do nothing if we stopped.
  if not Monitoring then
    Exit;

  EnterCriticalSection(FConnectionsCS);
  try
    // Make sure that device is not in connections list.
    if FPendingConnections.Contains(Address) then
      Exit;

    // Check devices name.
    if Name = DEVICE_NAME then begin
      // Notify about new device.
      DoDeviceFound(Address, Name);

      // Create client.
      Client := TGattClient.Create;
      // Set required event handlers.
      Client.OnCharacteristicChanged := ClientCharacteristicChanged;
      Client.OnConnect := ClientConnect;
      Client.OnDisconnect := ClientDisconnect;
      // Try to start connection to the device.
      Result := Client.Connect(Address, Radio);
      // Report connection start event.
      DoConnectionStarted(Address, Result);
      // If connection started with success...
      if Result = WCL_E_SUCCESS then
        // ...add device to pending connections list.
        FPendingConnections.Add(Address)
      else
        Client.Free;
    end;
  finally
    LeaveCriticalSection(FConnectionsCS);
  end;
end;

procedure TClientWatcher.DoClientDisconnected(const Address: Int64;
  const Reason: Integer);
begin
  if Assigned(FOnClientDisconnected) then
    FOnClientDisconnected(Address, Reason);
end;

procedure TClientWatcher.DoConnectionCompleted(const Address: Int64;
  const Result: Integer);
begin
  if Assigned(FOnConnectionCompleted) then
    FOnConnectionCompleted(Address, Result);
end;

procedure TClientWatcher.DoConnectionStarted(const Address: Int64;
  const Result: Integer);
begin
  if Assigned(FOnConnectionStarted) then
    FOnConnectionStarted(Address, Result);
end;

procedure TClientWatcher.DoDeviceFound(const Address: Int64;
  const Name: string);
begin
  if Assigned(FOnDeviceFound) then
    FOnDeviceFound(Address, Name);
end;

procedure TClientWatcher.DoStopped;
var
  Clients: TList<TGattClient>;
  Client: TGattClient;
begin
  Clients := TList<TGattClient>.Create;
  try
    EnterCriticalSection(FConnectionsCS);
    try
      if FConnections.Count > 0 then begin
        // Make copy of the connected clients.
        for Client in FConnections do
          Clients.Add(Client);
      end;
    finally
      LeaveCriticalSection(FConnectionsCS);
    end;

    // Disconnect all connected clients.
    if Clients.Count > 0 then begin
      for Client in Clients do
        Client.Disconnect;
    end;
  finally
    Clients.Free;
  end;

  inherited DoStopped;
end;

procedure TClientWatcher.DoValueChanged(const Address: Int64;
  const Value: TwclGattCharacteristicValue);
begin
  if Assigned(FOnValueChanged) then
    FOnValueChanged(Address, Value);
end;

function TClientWatcher.FindClient(const Address: Int64): TGattClient;
var
  Client: TGattClient;
begin
  Result := nil;
  if FConnections.Count > 0 then begin
    for Client in FConnections do begin
      if Client.Address = Address then begin
        Result := Client;
        Break;
      end;
    end;
  end;
end;

function TClientWatcher.ReadData(const Address: Int64;
  out Data: TwclGattCharacteristicValue): Integer;
var
  Client: TGattClient;
begin
  Data := nil;

  if not Monitoring then
    Result := WCL_E_CONNECTION_CLOSED

  else begin
    EnterCriticalSection(FConnectionsCS);
    try
      Client := FindClient(Address);
      if Client = nil then
        Result := WCL_E_CONNECTION_NOT_ACTIVE
      else
        Result := Client.ReadValue(Data);
    finally
      LeaveCriticalSection(FConnectionsCS);
    end;
  end;
end;

procedure TClientWatcher.RemoveClient(const Client: TGattClient);
begin
  if Client = nil then
    Exit;

  EnterCriticalSection(FConnectionsCS);
  try
    // Remove client from the connections list.
    if FPendingConnections.Contains(Client.Address) then begin
      Client.OnCharacteristicChanged := nil;
      Client.OnConnect := nil;
      Client.OnDisconnect := nil;
      FPendingConnections.Remove(Client.Address);
    end;

    // Remove client from the clients list.
    if FConnections.Contains(Client) then
      FConnections.Remove(Client);

    SetOldClient(Client);

  finally
    LeaveCriticalSection(FConnectionsCS);
  end;
end;

procedure TClientWatcher.SetOldClient(const Client: TGattClient);
begin
  EnterCriticalSection(FConnectionsCS);
  try
    if FOldClient <> nil then begin
      FOldClient.Free;
      FOldClient := nil;
    end;

    FOldClient := Client;

  finally
    LeaveCriticalSection(FConnectionsCS);
  end;
end;

function TClientWatcher.WriteData(const Address: Int64;
  const Data: TwclGattCharacteristicValue): Integer;
var
  Client: TGattClient;
begin
  if not Monitoring then
    Result := WCL_E_CONNECTION_CLOSED

  else begin
    if Length(Data) = 0 then
      Result := WCL_E_INVALID_ARGUMENT

    else begin
      EnterCriticalSection(FConnectionsCS);
      try
        Client := FindClient(Address);
        if Client = nil then
          Result := WCL_E_CONNECTION_NOT_ACTIVE
        else
          Result := Client.WriteValue(Data);
      finally
        LeaveCriticalSection(FConnectionsCS);
      end;
    end;
  end;
end;

end.
