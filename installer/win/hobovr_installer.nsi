# plugins required:
# 	Inetc: https://nsis.sourceforge.io/Inetc_plug-in
# 	Nsisunz: https://nsis.sourceforge.io/Nsisunz_plug-in
#	ExecDos: https://nsis.sourceforge.io/ExecDos_plug-in
#	Locate: https://nsis.sourceforge.io/Locate_plugin
#   Download the zip files from each of those and copy the contents of the plugin.zip/Plugins directory to program files/NSIS/plugins
!include nsDialogs.nsh
!include MUI2.nsh
!include "winmessages.nsh"
!include LogicLib.nsh
!include "Locate.nsh"

#---------------------------------------------------------------------------------------------------

; https://nsis.sourceforge.io/StrContains
; StrContains
; This function does a case sensitive searches for an occurrence of a substring in a string. 
; It returns the substring if it is found. 
; Otherwise it returns null(""). 
; Written by kenglish_hi
; Adapted from StrReplace written by dandaman32
 
Var STR_HAYSTACK
Var STR_NEEDLE
Var STR_CONTAINS_VAR_1
Var STR_CONTAINS_VAR_2
Var STR_CONTAINS_VAR_3
Var STR_CONTAINS_VAR_4
Var STR_RETURN_VAR
 
Function StrContains
  Exch $STR_NEEDLE
  Exch 1
  Exch $STR_HAYSTACK
  ; Uncomment to debug
  ;MessageBox MB_OK 'STR_NEEDLE = $STR_NEEDLE STR_HAYSTACK = $STR_HAYSTACK '
    StrCpy $STR_RETURN_VAR ""
    StrCpy $STR_CONTAINS_VAR_1 -1
    StrLen $STR_CONTAINS_VAR_2 $STR_NEEDLE
    StrLen $STR_CONTAINS_VAR_4 $STR_HAYSTACK
    loop:
      IntOp $STR_CONTAINS_VAR_1 $STR_CONTAINS_VAR_1 + 1
      StrCpy $STR_CONTAINS_VAR_3 $STR_HAYSTACK $STR_CONTAINS_VAR_2 $STR_CONTAINS_VAR_1
      StrCmp $STR_CONTAINS_VAR_3 $STR_NEEDLE found
      StrCmp $STR_CONTAINS_VAR_1 $STR_CONTAINS_VAR_4 done
      Goto loop
    found:
      StrCpy $STR_RETURN_VAR $STR_NEEDLE
      Goto done
    done:
   Pop $STR_NEEDLE ;Prevent "invalid opcode" errors and keep the
   Exch $STR_RETURN_VAR  
FunctionEnd
 
!macro _StrContainsConstructor OUT NEEDLE HAYSTACK
  Push `${HAYSTACK}`
  Push `${NEEDLE}`
  Call StrContains
  Pop `${OUT}`
!macroend
 
!define StrContains '!insertmacro "_StrContainsConstructor"'

#---------------------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------------------

; FindPython - SimLeek
; Searches for Python 3.7-3.9 and returns the EXE and Version if found.

Var pythonExe
Var pythonVer
Var minPythonMinorVer
Var pythonExeDownloaded

Function FindPython
	ExecDos::exec /TOSTACK /TIMEOUT=2000 "where py"
	Pop $1				
	Pop $1				

	StrCpy $pythonExe $1
	${StrContains} $0 "INFO" $pythonExe
	StrCmp $0 "" foundpy
		#MessageBox MB_OK 'Found string $0'
			${locate::Open} "C:\Python38" "/L=F /M=python.exe /S=1K" $0
			StrCmp $0 0 0 foundcpy
				${locate::Close} $0
				${locate::Open} "C:\Python39" "/L=F /M=python.exe /S=1K" $0
				StrCmp $0 0 0 foundcpy
					${locate::Close} $0
					${locate::Open} "C:\Python37" "/L=F /M=python.exe /S=1K" $0
					StrCmp $0 0 0 foundcpy
						${locate::Close} $0
						${locate::Open} "$APPDATA\Local\Programs\Python" "/L=F /M=python.exe /S=1K" $0
							StrCmp $0 0 0 foundcpy
								MessageBox MB_OK "Python could not be found. Please locate or install it."
								Goto closebad
			foundcpy:
				${locate::Find} $0 $1 $2 $3 $4 $5 $6
				StrCpy $pythonExe $0
				Goto done
		  Goto done
		foundpy:
			StrCpy $pythonExe "py -3"
			#MessageBox MB_OK 'Found command line python'
		done:
		
		#check that we have the right version
		ExecDos::exec /TOSTACK /TIMEOUT=2000 "$pythonExe -V"
		Pop $1
		#MessageBox MB_OK 'pyver: $1'
		Pop $1
		#MessageBox MB_OK 'pyver: $1'
		StrCpy $pythonVer $1 1 9
		#MessageBox MB_OK 'Found minor ver $pythonVer'
		${If} $pythonVer > $minPythonMinorVer
			#MessageBox MB_OK "Correct Python version found: 3.$pythonVer."
			Goto closegood
		${Else}
			MessageBox MB_OK "Incorrect Python version found: 3.$pythonVer. Installing correct one."
			Goto closebad
		${EndIf}
		Goto closegood
		
		closebad:
			${locate::Close} $0
			${locate::Unload}
			StrCpy $pythonExe "false"
			StrCpy $pythonVer "false"
			Return
		closegood:
			${locate::Close} $0
			${locate::Unload}
			Return
FunctionEnd

Function EnsurePython
	Call FindPython
	StrCmp $pythonExe "false" noPython
	Goto pydone
	noPython:
		inetc::get "https://www.python.org/ftp/python/3.8.10/python-3.8.10-amd64.exe" "$TEMP\python-3.8.10-amd64.exe" $pythonExeDownloaded
		Pop $R0
		StrCpy $pythonExeDownloaded $R0
		StrCmp $pythonExeDownloaded "OK" python_downloaded python_not_downloaded
		python_not_downloaded:
			MessageBox MB_OK|MB_ICONEXCLAMATION "Unable to download python: $pythonExeDownloaded."
			Quit
		python_downloaded:
			MessageBox MB_OK|MB_ICONEXCLAMATION "Python downloaded. Please install with 'Add Python to PATH' checked."
			ExecDos::exec "$TEMP\python-3.8.10-amd64.exe"
			Call FindPython
			StrCmp $pythonExe "" failPython
				Goto pydone
			failPython:
				MessageBox MB_OK|MB_ICONEXCLAMATION "Python was not installed correctly. Please install Python 3.8 and re-launch the installer."
				Quit
	pydone:
FunctionEnd

#---------------------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------------------

; https://nsis.sourceforge.io/NsDialogs_FAQ#How_to_create_two_groups_of_RadioButtons

Var dialog
Var hwnd
Var isexpress

Function preExpressCustom
	nsDialogs::Create 1018
		Pop $dialog
 
	${NSD_CreateRadioButton} 0 0 40% 6% "Express"
		Pop $hwnd
		${NSD_AddStyle} $hwnd ${WS_GROUP}
		nsDialogs::SetUserData $hwnd "true"
		${NSD_OnClick} $hwnd RadioClick
		${NSD_Check} $hwnd
	${NSD_CreateRadioButton} 0 12% 40% 6% "Custom"
		Pop $hwnd
		nsDialogs::SetUserData $hwnd "false"
		${NSD_OnClick} $hwnd RadioClick
 
	nsDialogs::Show
FunctionEnd
 
Function RadioClick
	Pop $hwnd
	nsDialogs::GetUserData $hwnd
	Pop $hwnd
	StrCpy $isexpress $hwnd
FunctionEnd

#---------------------------------------------------------------------------------------------------

Name "HoboVR"
OutFile "hobovr_setup_web.exe"
XPStyle on


Var EXIT_CODE
Var HobovrZipDownloaded

!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
#Page custom preExpressCustom 
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Function .onInit
  StrCpy $isexpress "true"
  StrCpy $INSTDIR "$PROGRAMFILES\Hobo VR"
  StrCpy $minPythonMinorVer 7
FunctionEnd


Section

CreateDirectory $INSTDIR
AccessControl::GrantOnFile  "$INSTDIR" "(S-1-5-32-545)" "FullAccess"

Call EnsurePython

# Download and Unzip HoboVR Master branch to install directory
inetc::get "https://github.com/okawo80085/hobo_vr/archive/refs/heads/master.zip" "$TEMP\hobovr_master.zip" $HobovrZipDownloaded
Pop $R0
StrCpy $HobovrZipDownloaded $R0
StrCmp $HobovrZipDownloaded "OK" 0 hobo_download_failed
nsisunz::Unzip "$TEMP\hobovr_master.zip" "$INSTDIR"
	
# Tell Steam where the VR DLLs are
IfFileExists "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" PathGood
	MessageBox MB_OK|MB_ICONEXCLAMATION "Steam vr path register exe doesn't exist. has steam and steamvr been installed?"
	Quit
PathGood:
	ExecDos::exec /DETAILED '"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" adddriver "$INSTDIR\hobo_vr-master\hobovr"' $EXIT_CODE install_log.txt
	ExecDos::exec /DETAILED '"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" show' $EXIT_CODE install_log.txt

# Install hobovr virtualreality python bindings
ExecDos::exec /DETAILED '$pythonExe -m pip install -e "$INSTDIR\hobo_vr-master\bindings\python"' $EXIT_CODE install_log.txt
goto end
	
# Handle errors
hobo_download_failed:
	MessageBox MB_OK|MB_ICONEXCLAMATION "Unable to download hobo_vr: $HobovrZipDownloaded."
	 Quit
end:

SectionEnd