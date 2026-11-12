#define AppName "C-Deck"
#define AppVersion "1.3.0"
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

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "Criar atalho na area de trabalho"; GroupDescription: "Atalhos adicionais:"; Flags: unchecked

[Files]
Source: "..\build\C-Deck.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\config\apps.cfg"; DestDir: "{app}\config"; Flags: onlyifdoesntexist
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\C-Deck"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\C-Deck"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Abrir C-Deck"; Flags: postinstall nowait skipifsilent
