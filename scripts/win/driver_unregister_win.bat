@echo off
IF EXIST "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\" (
IF EXIST "%~dp0..\..\hobovr" (
	set old_pwd="%~dp0"
	cd  "..\..\hobovr"

rem fuck windows, this shit is disgusting, you need a fucking for loop to capture an output of a command wtf
	FOR /F "tokens=* USEBACKQ" %%F IN (`cd`) DO (
	SET var=%%F
	)
	cd %old_pwd%
	"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" removedriver "%var%"
	"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" show
) ELSE (
	echo "could not find driver, is the repository broken?"
)
) ELSE (
echo "SteamVR not located in C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR - Uninstallation Failed"
)
pause
