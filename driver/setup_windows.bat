@echo off

if exist %USERPROFILE%/vcpkg/scripts/buildsystems/vcpkg.cmake (
	echo vcpkg already installed
 ) else (
	echo vcpkg not installed. Installing...
	pushd %USERPROFILE%
	git clone https://github.com/microsoft/vcpkg
	.\vcpkg\bootstrap-vcpkg.bat
	popd
)

echo Installing OpenVR...
pushd %USERPROFILE%
.\vcpkg\vcpkg install openvr:x64-windows openvr zeromq:x64-windows zeromq cppzmq cppzmq:x64-windows protobuf[zlib] protobuf[zlib]:x64-windows
REM for testing:
.\vcpkg\vcpkg install pybind11 pybind11:x64-windows catch2 catch2:x64-windows
popd