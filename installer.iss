; =========================
; BASIC
; =========================
#define MyAppName "SimplePrinter"
#define MyAppVersion "1"
#define MyAppPublisher "ngth1010v"
#define MyAppURL "https://github.com/ngth1010v"
#define MyAppExeName "SimplePrinter.exe"

[Setup]
AppId={{F8FFDA3A-DD85-4094-9F14-0BA4DADA2BBC}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

DefaultDirName={autopf}\{#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

PrivilegesRequired=lowest
DisableProgramGroupPage=yes

OutputBaseFilename=Simple-printer-installer
SolidCompression=yes
WizardStyle=modern

CloseApplications=yes
RestartApplications=no
LicenseFile=LICENSE


[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "SimplePrinter.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "SimplePrinterLauncher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "pdfium.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "assets\*"; DestDir: "{app}\assets"; Flags: recursesubdirs createallsubdirs

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Registry]

; =========================
; OPEN WITH SIMPLEPRINTER
; =========================
Root: HKCU; Subkey: "Software\Classes\*\shell\SimplePrinter.Open"; ValueType: string; ValueName: ""; ValueData: "Open with SimplePrinter"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\*\shell\SimplePrinter.Open"; ValueType: string; ValueName: "Icon"; ValueData: "{app}\SimplePrinter.exe"
Root: HKCU; Subkey: "Software\Classes\*\shell\SimplePrinter.Open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\SimplePrinterLauncher.exe"" ""%1"""

; =========================
; PRINT WITH SIMPLEPRINTER
; =========================
Root: HKCU; Subkey: "Software\Classes\*\shell\SimplePrinter.Print"; ValueType: string; ValueName: ""; ValueData: "Print with SimplePrinter"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\*\shell\SimplePrinter.Print"; ValueType: string; ValueName: "Icon"; ValueData: "{app}\SimplePrinter.exe"
Root: HKCU; Subkey: "Software\Classes\*\shell\SimplePrinter.Print\command"; ValueType: string; ValueName: ""; ValueData: """{app}\SimplePrinterLauncher.exe"" --print ""%1"""

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"