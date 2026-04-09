#define AppName "Milena MBROLA SAPI5"
#define AppVersion "1.0.0"
#define AppPublisher "milena contributors"

[Setup]
AppId={{BF85F696-CCEF-4A8C-B2B0-C73D94848A5E}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\Milena MBROLA SAPI5
DefaultGroupName={#AppName}
OutputDir=..\dist\installer
OutputBaseFilename=MilenaMBROLA-SAPI5-{#AppVersion}-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"

[Files]
Source: "..\sapi5_milena\dist\x64\MilenaSapi5.dll"; DestDir: "{app}\x64"; Flags: ignoreversion
Source: "..\sapi5_milena\dist\x86\MilenaSapi5.dll"; DestDir: "{app}\x86"; Flags: ignoreversion
Source: "..\sapi5_milena\dist\milena.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\sapi5_milena\dist\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\sapi5_milena\dist\mbrola\*"; DestDir: "{app}\mbrola"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\register_milena_sapi5.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\unregister_milena_sapi5.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\test_milena_sapi5.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Odinstaluj Milena MBROLA SAPI5"; Filename: "{uninstallexe}"

[Run]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\register_milena_sapi5.ps1"" -MachineWide -InstallDir ""{app}"""; Flags: runhidden waituntilterminated

[UninstallRun]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\unregister_milena_sapi5.ps1"" -MachineWide"; Flags: runhidden waituntilterminated; RunOnceId: "UnregisterMilenaSapi5"
