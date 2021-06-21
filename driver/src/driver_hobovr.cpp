//#include "openvr.h"
#include "openvr_driver.h"
//#include "openvr_capi.h"
#include "driverlog.h"

#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <variant>

#if defined(_WINDOWS)
#include <windows.h>
#endif

#if defined(_WIN32)
	//dll filesystem info
	#include "libloaderapi.h"
	#include <string>
	#include <filesystem>
	namespace fs = std::filesystem;
#elif defined(__linux__)
	#define _stricmp strcasecmp
#endif

using namespace vr;

#if defined(_WIN32)
	#define HMD_DLL_EXPORT extern "C" __declspec(dllexport)
	#define HMD_DLL_IMPORT extern "C" __declspec(dllimport)
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
	#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
	#define HMD_DLL_IMPORT extern "C"
#else
	#error "Unsupported Platform."
#endif

#include "server/zmqreqrep.h"

/*ServerDriverHoboVR global_hobovr_driver;

//-----------------------------------------------------------------------------
// Purpose: driverFactory
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void* HmdDriverFactory(const char* pInterfaceName,
	int* pReturnCode) {
	if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName)) {
		return &global_hobovr_driver;
	}

	if (pReturnCode)
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}*/