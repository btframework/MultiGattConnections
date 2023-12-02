unit GattClient;

interface

uses
  wclBluetooth, Windows;

type
  TGattClient = class(TwclGattClient)
  private const
    {$REGION Attribute UUIDs}
    SERVICE_UUID: TGUID = '{a25fede7-395c-4241-adc0-481004d81900}';

    NOTIFIABLE_CHARACTERISTIC_UUID: TGUID = '{eabbbb91-b7e5-4e50-b7aa-ced2be8dfbbe}';
    READABLE_CHARACTERISTIC_UUID: TGUID = '{468dfe19-8de3-4181-b728-0902c50a5e6d}';
    WRITABLE_CHARACTERISTIC_UUID: TGUID = '{421754b0-e70a-42c9-90ed-4aed82fa7ac0}';
    {$ENDREGION Attribute UUIDs}

  private
    {$REGION Private fields}
    FConnected: Boolean;
    FCS: RTL_CRITICAL_SECTION;

    {$REGION Attributes}
    FReadableChar: TwclGattCharacteristic;
    FWritableChar: TwclGattCharacteristic;
    {$ENDREGION Attributes}
    {$ENDREGION Private fields}

  protected
    {$REGION GATT Client overrides}
    // The method called when connection procedure completed (with or without
    // success). If the Error parameter is WCL_E_SUCCESS then we are connected
    // and can read attributes and subscribe. If something goes wrong during
    // attributes reading and subscribing we have to disconnect from the device.
    // If the Error parameter is not WCL_E_SUCCESS then connection failed.
    // Simple call OnConnect event with error and do nothing more.
    procedure DoConnect(const Error: Integer); override;
    // The method called when the remote device disconnected. The Reason
    // parameter indicates disconnection reason.
    procedure DoDisconnect(const Reason: Integer); override;
    {$ENDREGION GATT Client overrides}

  public
    {$REGION Constructor and Destructor}
    constructor Create; reintroduce;
    destructor Destroy; override;
    {$ENDREGION Constructor and Destructor}

    {$REGION Connection and disconnection}
    // Override connect method. We need it for thread synchronization.
    function Connect(const Address: Int64;
      const Radio: TwclBluetoothRadio): Integer;
    // Override disconnect method. We need it for thread synchronization.
    function Disconnect: Integer;
    {$ENDREGION Connection and disconnection}

    {$REGION Reading and writing values}
    // Simple read value from the readable characteristic.
    function ReadValue(out Value: TwclGattCharacteristicValue): Integer;
    // Simple write value to the writable characteristic.
    function WriteValue(const Value: TwclGattCharacteristicValue): Integer;
    {$ENDREGION Reading and writing values}
  end;

implementation

uses
  wclErrors, wclConnections, wclConnectionErrors;

{ TGattClient }

function TGattClient.Connect(const Address: Int64;
  const Radio: TwclBluetoothRadio): Integer;
begin
  if (Address = 0) or (Radio = nil) then
    Result := WCL_E_INVALID_ARGUMENT

  else begin
    EnterCriticalSection(FCS);
    try
      if FConnected or (State <> csDisconnected) then
        Result := WCL_E_CONNECTION_ACTIVE

      else begin
        Self.Address := Address;
        Result := inherited Connect(Radio);
      end;
    finally
      LeaveCriticalSection(FCS);
    end;
  end;
end;

constructor TGattClient.Create;
begin
  inherited Create(nil);

  FConnected := False;
  InitializeCriticalSection(FCS);
end;

destructor TGattClient.Destroy;
begin
  // We have to call disconnect here, otherwise when inherited destructor
  // will be called we will get crash because all the objects are already
  // destroyed.
  Disconnect;

  // Now destroy the objects.
  DeleteCriticalSection(FCS);

  // And now we are safe to call inherited destructor.
  inherited;
end;

function TGattClient.Disconnect: Integer;
begin
  EnterCriticalSection(FCS);
  try
    if not FConnected then
      Result := WCL_E_CONNECTION_NOT_ACTIVE
    else
     Result := inherited Disconnect;
  finally
    LeaveCriticalSection(FCS);
  end;
end;

procedure TGattClient.DoConnect(const Error: Integer);
var
  Res: Integer;
  Uuid: TwclGattUuid;
  Service: TwclGattService;
  NotifiableChar: TwclGattCharacteristic;
begin
  // Copy it so we can change it.
  Res := Error;
  // If connection was success try to read required attributes.
  if Res = WCL_E_SUCCESS then begin
    Uuid.IsShortUuid := False;

    // First find required service.
    Uuid.LongUuid := SERVICE_UUID;
    Res := FindService(Uuid, Service);
    if Res = WCL_E_SUCCESS then begin
      // Service found. Try to find readable characteristic.
      Uuid.LongUuid := READABLE_CHARACTERISTIC_UUID;
      Res := FindCharacteristic(Service, Uuid, FReadableChar);
      if Res = WCL_E_SUCCESS then begin
        // If readable characteristic found try to find writable one.
        Uuid.LongUuid := WRITABLE_CHARACTERISTIC_UUID;
        Res := FindCharacteristic(Service, Uuid, FWritableChar);
        if Res = WCL_E_SUCCESS then begin
          // Ok, we got writable characteristic. Now try to find notifiable one.
          Uuid.LongUuid := NOTIFIABLE_CHARACTERISTIC_UUID;
          Res := FindCharacteristic(Service, Uuid, NotifiableChar);
          if Res = WCL_E_SUCCESS then begin
            // Notifiable characteristic found. Try to subscribe. We do not need
            // to save the characteristic globally. It will be unsubscribed
            // during disconnection.
            Res := SubscribeForNotifications(NotifiableChar);

            // If subscribed - set connected flag.
            if Res = WCL_E_SUCCESS then begin
              EnterCriticalSection(FCS);
              try
                FConnected := True;
              finally
                LeaveCriticalSection(FCS);
              end;
            end;
          end;
        end;
      end;
    end;

    // If something went wrong we must disconnect!
    if Res <> WCL_E_SUCCESS then
      Disconnect;
  end;

  // Call inherited method anyway so the OnConnect event fires.
  inherited DoConnect(Res);
end;

procedure TGattClient.DoDisconnect(const Reason: Integer);
begin
  // Make sure that we were "connected".
  if FConnected then begin
    // Simple cleanup.
    EnterCriticalSection(FCS);
    try
      FConnected := False;
    finally
      LeaveCriticalSection(FCS);
    end;
  end;

  // Call the inherited method to fire the OnDisconnect event.
  inherited DoDisconnect(Reason);
end;

function TGattClient.ReadValue(out Value: TwclGattCharacteristicValue): Integer;
begin
  Value := nil;

  EnterCriticalSection(FCS);
  try
    if not FConnected then
      Result := WCL_E_CONNECTION_CLOSED
    else
      // Always use Read From Device.
      Result := ReadCharacteristicValue(FReadableChar, goReadFromDevice, Value);
  finally
    LeaveCriticalSection(FCS);
  end;
end;

function TGattClient.WriteValue(
  const Value: TwclGattCharacteristicValue): Integer;
begin
  if Length(Value) = 0 then
    Result := WCL_E_INVALID_ARGUMENT

  else begin
    EnterCriticalSection(FCS);
    try
      if not FConnected then
        Result := WCL_E_CONNECTION_CLOSED
      else
        Result := WriteCharacteristicValue(FWritableChar, Value);
    finally
      LeaveCriticalSection(FCS);
    end;
  end;
end;

end.
