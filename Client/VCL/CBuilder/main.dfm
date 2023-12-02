object fmMain: TfmMain
  Left = 0
  Top = 0
  BorderStyle = bsSingle
  Caption = 'Multy GATT Client Test'
  ClientHeight = 460
  ClientWidth = 810
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object laData: TLabel
    Left = 423
    Top = 48
    Width = 62
    Height = 13
    Caption = 'Data to send'
  end
  object btStart: TButton
    Left = 8
    Top = 8
    Width = 75
    Height = 25
    Action = acStart
    TabOrder = 0
  end
  object btStop: TButton
    Left = 89
    Top = 8
    Width = 75
    Height = 25
    Action = acStop
    TabOrder = 1
  end
  object lvDevices: TListView
    Left = 8
    Top = 39
    Width = 409
    Height = 150
    Columns = <
      item
        Caption = 'Address'
        Width = 120
      end
      item
        Caption = 'Name'
        Width = 120
      end
      item
        Caption = 'Status'
        Width = 120
      end>
    GridLines = True
    HideSelection = False
    ReadOnly = True
    RowSelect = True
    TabOrder = 2
    ViewStyle = vsReport
  end
  object btDisconnect: TButton
    Left = 342
    Top = 8
    Width = 75
    Height = 25
    Action = acDisconnect
    TabOrder = 3
  end
  object edData: TEdit
    Left = 491
    Top = 45
    Width = 311
    Height = 21
    TabOrder = 4
  end
  object btRead: TButton
    Left = 491
    Top = 72
    Width = 75
    Height = 25
    Action = acRead
    TabOrder = 5
  end
  object btWrite: TButton
    Left = 727
    Top = 72
    Width = 75
    Height = 25
    Action = acWrite
    TabOrder = 6
  end
  object lbLog: TListBox
    Left = 8
    Top = 195
    Width = 794
    Height = 257
    ItemHeight = 13
    TabOrder = 7
  end
  object btClear: TButton
    Left = 727
    Top = 164
    Width = 75
    Height = 25
    Caption = 'Clear'
    TabOrder = 8
    OnClick = btClearClick
  end
  object ActionList: TActionList
    Left = 312
    Top = 264
    object acStart: TAction
      Caption = 'Start'
      OnExecute = acStartExecute
      OnUpdate = acStartUpdate
    end
    object acStop: TAction
      Caption = 'Stop'
      OnExecute = acStopExecute
      OnUpdate = acStopUpdate
    end
    object acDisconnect: TAction
      Caption = 'Disconnect'
      OnExecute = acDisconnectExecute
      OnUpdate = acDisconnectUpdate
    end
    object acRead: TAction
      Caption = 'Read'
      OnExecute = acReadExecute
      OnUpdate = acReadUpdate
    end
    object acWrite: TAction
      Caption = 'Write'
      OnExecute = acWriteExecute
      OnUpdate = acWriteUpdate
    end
  end
  object Manager: TwclBluetoothManager
    AfterOpen = ManagerAfterOpen
    BeforeClose = ManagerBeforeClose
    OnClosed = ManagerClosed
    Left = 512
    Top = 136
  end
end
