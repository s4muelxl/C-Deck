#define AppName "C-Deck"
#define AppVersion "1.3.1"
#define AppPublisher "C-Deck"
#define AppExeName "C-Deck.exe"

[Setup]
AppId={{A4F1B25D-8A73-4F8E-9A2D-4C6E8B9F1D30}}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\C-Deck
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
OutputDir=..\build\installer
OutputBaseFilename=C-Deck-Setup
SetupIconFile=..\build\cdeck.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; Use Windows Restart Manager to close C-Deck before replacing its executable.
CloseApplications=force
CloseApplicationsFilter=*.exe
RestartApplications=no

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "Criar atalho na area de trabalho"; GroupDescription: "Atalhos adicionais:"; Flags: unchecked

[Files]
Source: "..\build\C-Deck.exe"; DestDir: "{app}"; Flags: ignoreversion restartreplace
Source: "..\config\apps.cfg"; DestDir: "{app}\config"; Flags: onlyifdoesntexist
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\C-Deck"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\C-Deck"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Abrir C-Deck"; Flags: postinstall nowait skipifsilent

[Code]
// Close running instance of C-Deck before replacing files
function InitializeSetup(): Boolean;
var
  Wnd: HWND;
  CloseMsg: UINT;
  I: Integer;
begin
  Result := True;
  Wnd := FindWindowByClassName('CDeckMainWindow');
  if Wnd <> 0 then
  begin
    CloseMsg := RegisterWindowMessage('CDeck_CloseMessage');
    if CloseMsg <> 0 then
      PostMessage(Wnd, CloseMsg, 0, 0);
    PostMessage(Wnd, $0111 { WM_COMMAND }, 3002 { ID_TRAY_EXIT }, 0);
    for I := 0 to 10 do
    begin
      Sleep(100);
      if FindWindowByClassName('CDeckMainWindow') = 0 then
        Break;
    end;
  end;
end;
