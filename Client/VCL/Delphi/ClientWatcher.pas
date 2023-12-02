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
    {$REGION Connections management}
    FConnectionsCS: RTL_CRITICAL_SECTION;
    FClients: TList<TGattClient>; // Connected clients.
    FConnections: TDictionary<Int64, TGattClient>; // Pending connections.
    FFoundDevices: TList<Int64>;
    FTempClient: TGattClient; // Used to store client that must be destroyed.
    {$ENDREGION Connections management}

    {$REGION Events}
    FOnClientDisconnected: TClientDisconnected;
    FOnConnectionCompleted: TClientConnectionCompleted;
    FOnConnectionStarted: TClientConnectionStarted;
    FOnDeviceFound: TClientDeviceFound;
    FOnValueChanged: TClientValueChanged;
    {$ENDREGION Events}

    {$REGION Helper method}
    procedure DestroyClient(const Client: TGattClient);
    procedure RemoveClient(const Client: TGattClient);
    {$ENDREGION Helper method}

    {$REGION Client event handlers}
    procedure ClientDisconnect(Sender: TObject; const Reason: Integer);
    procedure ClientConnect(Sender: TObject; const Error: Integer);
    procedure ClientCharacteristicChanged(Sender: TObject; const Handle: Word;
      const Value: TwclGattCharacteristicValue);
    {$ENDREGION Client event handlers}

    {$REGION Events management}
    procedure DoClientDisconnected(const Address: Int64; const Reason: Integer);
    procedure DoConnectionCompleted(const Address: Int64;
      const Result: Integer);
    procedure DoConnectionStarted(const Address: Int64; const Result: Integer);
    procedure DoDeviceFound(const Address: Int64; const Name: string);
    procedure DoValueChanged(const Address: Int64;
      const Value: TwclGattCharacteristicValue);
    {$ENDREGION Events management}

  protected
    {$REGION Device search handling}
    procedure DoAdvertisementFrameInformation(const Address: Int64;
      const Timestamp: Int64; const Rssi: ShortInt; const Name: string;
      const PacketType: TwclBluetoothLeAdvertisementType;
      const Flags: TwclBluetoothLeAdvertisementFlags); override;
    procedure DoStarted; override;
    procedure DoStopped; override;
    {$ENDREGION Device search handling}

  public
    {$REGION Constructor and destructor}
    constructor Create; reintroduce;
    destructor Destroy; override;
    {$ENDREGION Constructor and destructor}

    {$REGION Communication methods}
    function Disconnect(const Address: Int64): Integer;
    function ReadData(const Address: Int64;
      out Data: TwclGattCharacteristicValue): Integer;
    function WriteData(const Address: Int64;
      const Data: TwclGattCharacteristicValue): Integer;
    {$ENDREGION Communication methods}

    {$REGION Events}
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
    {$ENDREGION Events}
  end;

implementation

uses
  wclErrors, wclBluetoothErrors, wclConnectionErrors;

{ TClientWatcher }

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
      if not FConnections.ContainsKey(Address) then
        Result := WCL_E_BLUETOOTH_LE_DEVICE_NOT_FOUND

      else begin
        Client := FConnections[Address];
        if not FClients.Contains(Client) then
          Result := WCL_E_CONNECTION_NOT_ACTIVE
        else
          Result := Client.ReadValue(Data);
      end;
    finally
      LeaveCriticalSection(FConnectionsCS);
    end;
  end;
end;

procedure TClientWatcher.RemoveClient(const Client: TGattClient);
begin
  EnterCriticalSection(FConnectionsCS);
  try
    // Remove client from the connections list.
    if FConnections.ContainsKey(Client.Address) then begin
      Client.OnCharacteristicChanged := nil;
      Client.OnConnect := nil;
      Client.OnDisconnect := nil;
      FConnections.Remove(Client.Address);
    end;

    // Remove client from the clients list.
    if FClients.Contains(Client) then
      FClients.Remove(Client);
  finally
    LeaveCriticalSection(FConnectionsCS);
  end;

  DestroyClient(Client);
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
        if not FConnections.ContainsKey(Address) then
          Result := WCL_E_BLUETOOTH_LE_DEVICE_NOT_FOUND

        else begin
          Client := FConnections[Address];
          if not FClients.Contains(Client) then
            Result := WCL_E_CONNECTION_NOT_ACTIVE
          else
            Result := Client.WriteValue(Data);
        end;
      finally
        LeaveCriticalSection(FConnectionsCS);
      end;
    end;
  end;
end;

procedure TClientWatcher.ClientCharacteristicChanged(Sender: TObject;
  const Handle: Word; const Value: TwclGattCharacteristicValue);
var
  Client: TGattClient;
begin
  Client := TGattClient(Sender);
  // Simple call the value changed event.
  DoValueChanged(Client.Address, Value);
end;

procedure TClientWatcher.ClientConnect(Sender: TObject; const Error: Integer);
var
  Client: TGattClient;
  Address: Int64;
  Res: Integer;
begin
  Client := TGattClient(Sender);
  Address := Client.Address;
  // We need it to be able to change the value.
  Res := Error;

  // If we stopped we still can get client connection event.
  if not Monitoring then begin
    // Set the connection error code here.
    Res := WCL_E_BLUETOOTH_LE_CONNECTION_TERMINATED;
    // Disconnect client and set the connection error.
    Client.Disconnect;

  end else begin
    // If connection failed remove client from connections list.
    if Res <> WCL_E_SUCCESS then
      // Remove client from the lists.
      RemoveClient(Client)

    else begin
      // Othewrwise - add it to the connected clients list.
      EnterCriticalSection(FConnectionsCS);
      try
        FClients.Add(Client);
      finally
        LeaveCriticalSection(FConnectionsCS);
      end;
    end;
  end;

  // Call connection completed event.
  DoConnectionCompleted(Address, Res);
end;

procedure TClientWatcher.ClientDisconnect(Sender: TObject;
  const Reason: Integer);
var
  Client: TGattClient;
  Address: Int64;
begin
  Client := TGattClient(Sender);
  Address := Client.Address;

  // Remove client from the lists.
  RemoveClient(Client);
  // Call disconnect event.
  DoClientDisconnected(Address, Reason);
end;

constructor TClientWatcher.Create;
begin
  inherited Create(nil);

  InitializeCriticalSection(FConnectionsCS);

  FClients := TList<TGattClient>.Create;
  FConnections := TDictionary<Int64, TGattClient>.Create;
  FFoundDevices := TList<Int64>.Create;

  FTempClient := nil;

  FOnClientDisconnected := nil;
  FOnConnectionCompleted := nil;
  FOnConnectionStarted := nil;
  FOnDeviceFound := nil;
  FOnValueChanged := nil;
end;

destructor TClientWatcher.Destroy;
begin
  // We have to stop first! If we did not do it here than inherited destructor
  // calls Stop() and we wil crash because all objects are destroyed!
  Stop;

  DestroyClient(nil);

  // Now we can destroy the objects.
  DeleteCriticalSection(FConnectionsCS);

  FClients.Free;
  FConnections.Free;
  FFoundDevices.Free;

  // And now we are safe to call inherited destructor.
  inherited;
end;

procedure TClientWatcher.DestroyClient(const Client: TGattClient);
begin
  EnterCriticalSection(FConnectionsCS);
  try
    if FTempClient <> nil then begin
      FTempClient.Free;
      FTempClient := nil;
    end;

    FTempClient := Client;
  finally
    LeaveCriticalSection(FConnectionsCS);
  end;
end;

function TClientWatcher.Disconnect(const Address: Int64): Integer;
var
  Client: TGattClient;
begin
  if not Monitoring then
    Result := WCL_E_CONNECTION_CLOSED

  else begin
    Client := nil;

    EnterCriticalSection(FConnectionsCS);
    try
      if FConnections.ContainsKey(Address) then
        Client := FConnections[Address];

      if Client <> nil then begin
        if not FClients.Contains(Client) then
          Client := nil;
      end;
    finally
      LeaveCriticalSection(FConnectionsCS);
    end;

    if Client = nil then
      Result := WCL_E_CONNECTION_NOT_ACTIVE
    else
      Result := Client.Disconnect;
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
  if Monitoring then begin
    EnterCriticalSection(FConnectionsCS);
    try
      // Make sure that device is not in connections list.
      if not FConnections.ContainsKey(Address) then begin
        // Make sure that we did not see this device early.
        if not FFoundDevices.Contains(Address) then begin
          // Check devices name.
          if Name = DEVICE_NAME then begin
            // Add device into found list.
            FFoundDevices.Add(Address);
            // Call device found event.
            DoDeviceFound(Address, Name);
          end;
        end else begin
          // Device is in our devices list. Make sure we received connectable
          // advertisement.
          if (PacketType = atConnectableDirected) or
             (PacketType = atConnectableUndirected) then
          begin
            // We are ready to connect to the device.
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
              FConnections.Add(Address, Client)
            else
              // Otherwise - destroy the object.
              Client.Free;

            // Now we can remove the device from found devices list.
            FFoundDevices.Remove(Address);
          end;
        end;
      end;
    finally
      LeaveCriticalSection(FConnectionsCS);
    end;

    inherited DoAdvertisementFrameInformation(Address, Timestamp, Rssi, Name,
      PacketType, Flags);
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

procedure TClientWatcher.DoStarted;
begin
  // Clear all lists.
  FClients.Clear;
  FConnections.Clear;
  FFoundDevices.Clear;

  inherited DoStarted;
end;

procedure TClientWatcher.DoStopped;
var
  Clients: TList<TGattClient>;
  Client: TGattClient;
begin
  Clients := TList<TGattClient>.Create;

  EnterCriticalSection(FConnectionsCS);
  try
    if FClients.Count > 0 then begin
      // Make copy of the connected clients.
      for Client in FClients do
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

  Clients.Free;

  DestroyClient(nil);

  inherited DoStopped;
end;

procedure TClientWatcher.DoValueChanged(const Address: Int64;
  const Value: TwclGattCharacteristicValue);
begin
  if Assigned(FOnValueChanged) then
    FOnValueChanged(Address, Value);
end;

end.
