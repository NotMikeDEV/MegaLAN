!include "x64.nsh"
SetCompressor /SOLID lzma

; The name of the installer
Name "MegaLAN"

; The file to write
OutFile "MegaLAN-64bit.exe"

; The default installation directory
InstallDir $PROGRAMFILES64\MegaLAN

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_Example2" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages
LicenseData "LICENSE"
Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "MegaLAN (required)" ;No components page, name is not important
	WriteRegStr HKLM SOFTWARE\MegaLAN "Install_Dir" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MegaLAN" "DisplayName" "MegaLAN"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MegaLAN" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MegaLAN" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MegaLAN" "NoRepair" 1

	SetOutPath $INSTDIR
	SetOverwrite on
	CreateShortcut "$SMPROGRAMS\MegaLAN.lnk" "$INSTDIR\MegaLAN.exe" "" "$INSTDIR\MegaLAN.exe" 0

	File "x64\Release\MegaLAN.exe"
	File "Root\64bit\tapinstall.exe"
	File "Root\64bit\OemVista.inf"
	File "Root\64bit\tap0901.cat"
	File "Root\64bit\tap0901.sys"
	FileOpen $R0 "$INSTDIR\delete_tap.bat" w
	FileWrite $R0 '"$INSTDIR\tapinstall.exe" remove tap0901$\r$\n'
	FileClose $R0
	WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd ; end the section

Section "Visual Studio Runtimes"
	SetOutPath "$TEMP"
	File "Root\vc_redist.x64.exe"
	ExecWait "$TEMP\vc_redist.x64.exe /passive /norestart"
	Delete "$TEMP\vc_redist.x64.exe"
SectionEnd

Section "Uninstall"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MegaLAN"
	DeleteRegKey HKLM SOFTWARE\MegaLAN
	DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "MegaLAN"

	ExecWait "$INSTDIR\delete_tap.bat"
	Delete $INSTDIR\*.*
	Delete $INSTDIR\uninstall.exe

	Delete "$SMPROGRAMS\MegaLAN.lnk"
	Delete "$DESKTOP\MegaLAN.lnk"
	RMDir "$INSTDIR"
SectionEnd

Function .onInit
	SetRegView 64
FunctionEnd