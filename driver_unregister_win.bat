@echo off
IF EXIST "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\" (
"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" removedriver "%~dp0hobovr"
"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" show
) ELSE (
echo "SteamVR not located in C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR - Uninstallation Failed"
)
pause
