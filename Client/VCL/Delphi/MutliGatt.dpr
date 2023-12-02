program MutliGatt;

uses
  Vcl.Forms,
  main in 'main.pas' {fmMain},
  GattClient in 'GattClient.pas',
  ClientWatcher in 'ClientWatcher.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TfmMain, fmMain);
  Application.Run;
end.
