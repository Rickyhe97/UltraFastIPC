#define MyDllName "PE32Proxy"
#ifndef ApplicationVersion
#define ApplicationVersion GetFileVersion("..\PE32Proxy\bin\Debug\net7.0\"+MyDllName)
#endif

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId=21fc62df-51da-4ea5-8a6d-280b2994413f
AppName={#MyDllName}
AppVersion={#ApplicationVersion}
AppPublisher=InfiniTest Technologies, Ltd.
AppPublisherURL=http://www.infini-test.com/
AppSupportURL=http://www.infini-test.com/
AppUpdatesURL=http://www.infini-test.com/
DefaultDirName={commonpf64}\InfiniTest\PE32Proxy\
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputBaseFilename={#MyDllName} Installer {#ApplicationVersion}
SetupIconFile=Infinitest Icon.ico
Compression=lzma
SolidCompression=yes
UninstallDisplayIcon={app}\Infinitest Icon.ico
LicenseFile=license.rtf

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]     
Source: "..\PE32Proxy\bin\Debug\net7.0\*.*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
Source: "Infinitest Icon.ico"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

