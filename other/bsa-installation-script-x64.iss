; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "BSA Analytics"
#define MyAppVersion "1.0"
#define MyAppPublisher "vtyulb"
#define MyAppURL "bsa.vtyulb.ru"
#define MyAppExeName "BSA-Analytics.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{D08BFEE7-7DDB-4CBD-A5C4-B71C93420FBB}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf64}\BSA-Analytics
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=C:\BSA-Analytics\LICENSE
OutputBaseFilename=BSA-Analytics-x64
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "E:\work\bsa\BSA-Analytics-x64\BSA-Analytics.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\work\bsa\BSA-Analytics-x64\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKCR; Subkey: ".pnt"; ValueType: string; ValueName: ""; ValueData: "BSAShortData"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "BSAShortData"; ValueType: string; ValueName: ""; ValueData: "BSA Short Data"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "BSAShortData\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\BSA-Analytics.exe,0"
Root: HKCR; Subkey: "BSAShortData\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\BSA-Analytics.exe"" ""%1"""

Root: HKCR; Subkey: ".pnthr"; ValueType: string; ValueName: ""; ValueData: "BSALongData"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "BSALongData"; ValueType: string; ValueName: ""; ValueData: "BSA Long Data"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "BSALongData\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\BSA-Analytics.exe,0"
Root: HKCR; Subkey: "BSALongData\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\BSA-Analytics.exe"" ""%1"""

Root: HKCR; Subkey: ".pulsar"; ValueType: string; ValueName: ""; ValueData: "BSAPulsar"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "BSAPulsar"; ValueType: string; ValueName: ""; ValueData: "BSA-Analytics Pulsar"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "BSAPulsar\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\BSA-Analytics.exe,0"
Root: HKCR; Subkey: "BSAPulsar\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\BSA-Analytics.exe"" ""--analytics"" ""%1"""

Root: HKCR; Subkey: "Directory\shell\Run BSA-Analytics"; ValueType: string; ValueName: ""; ValueData: "Run BSA-Analytics"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "Directory\shell\Run BSA-Analytics\command"; ValueType: string; ValueName: ""; ValueData: """{app}\BSA-Analytics.exe"" ""--analytics"" ""%1"""
