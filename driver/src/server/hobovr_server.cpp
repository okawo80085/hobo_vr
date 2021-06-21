
#include "hobovr_server.h"

#include "zmqreqrep.h"
#include <openvr_driver.h>
#include <string>
#include <chrono>

#include "../device/hobovr_device_base.h"
#include "../driverlog.h"

#if defined(_WINDOWS)
#include <windows.h>
#endif

#include "protobufs/message.pb.h"


/************************
* Python Initialization *
************************/

//Currently Python initialization only works in windows, since SteamVR is mostly broken in Linux.
#if defined(_WIN32)
std::string get_module_path(void* address)
{
	//from: https://gist.github.com/pwm1234/05280cf2e462853e183d

	char path[FILENAME_MAX];
	HMODULE hm = NULL;

	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)address,
		&hm))
	{
		std::ostringstream oss;
		oss << "GetModuleHandle returned " << GetLastError();
		throw std::runtime_error(oss.str());

	}
	GetModuleFileNameA(hm, path, sizeof(path));

	std::string p = path;
	return p;
}

void noop()
{}

std::string path_to_my_dll()
{
	std::string str = get_module_path(noop);
	DriverLog("driver, python: module path: %s\n", str.c_str());
	return str;
}

std::string get_resources_path() {
	std::string p = path_to_my_dll();
	p = p.substr(0, p.find_last_of("/\\"));
	DriverLog("driver, python: dll path: %s\n", p.c_str());
	p = p.substr(0, p.find_last_of("/\\"));
	DriverLog("driver, python: bin path: %s\n", p.c_str());
	p = p.substr(0, p.find_last_of("/\\"));
	DriverLog("driver, python: driver path: %s\n", p.c_str());
	p += "\\resources\\";
	DriverLog("driver, python: resources path: %s\n", p.c_str());
	return p;
}

std::string get_python_start_file() {
	std::string p = get_resources_path();
	p += "on_start.py";
	DriverLog("driver, python: start python start file: %s\n", p.c_str());
	return p;
}

void start_python() {
	std::string str = "py -3 ";
	str += get_python_start_file();
	DriverLog("driver, python: start python command: %s\n", str.c_str());

	// the system command is blocking, so windows specific stuff is needed:
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;
	CreateProcessA(NULL, (LPSTR)str.c_str(), NULL, NULL, true, 0, NULL, NULL, &info, &processInfo);
}

#endif

bool on_packet(std::string& msg) {

}


vr::EVRInitError ServerDriverHoboVR::Init(vr::IVRDriverContext* pDriverContext) {
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);  //mock/comment out for testing
	InitDriverLog(vr::VRDriverLog()); // todo: make both stdout and steamvrout

	DriverLog("driver: version: %d.%d.%d %s \n", hobovr::k_nHobovrVersionMajor,
		hobovr::k_nHobovrVersionMinor,
		hobovr::k_nHobovrVersionBuild,
		hobovr::k_sHobovrVersionGG.c_str());

	//non vr stuff:
	this->start();
	DriverLog("started ZMQ rep server.");

#if defined(_WIN32)
	start_python();
#endif

	int counter_hmd = 0;
	int counter_cntrlr = 0;
	int counter_trkr = 0;
	int controller_hs = 1;

	// wait one second for python to tell us what devices we have before quitting
	using namespace std::chrono_literals;
	std::unique_lock<std::mutex> lk(cv_m);
	std::cerr << "Waiting... \n";
	if (cv.wait_for(lk, 1000ms) == std::cv_status::no_timeout) {
		DriverLog("Got device list back from ZMQ Server.");
	}
	else {
		DriverLog("Didn't get device list. Reporting no devices.");
	}

	for (std::string i : m_pSocketComm->m_vsDevice_list) {
		if (i == "h") {
			HeadsetDriver* temp = new HeadsetDriver("h" + std::to_string(counter_hmd));

			m_vDevices.push_back(temp);
			vr::VRServerDriverHost()->TrackedDeviceAdded(
				m_vDevices.back()->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD,
				temp);

			counter_hmd++;

		}
		else if (i == "c") {
			ControllerDriver* temp = new ControllerDriver(controller_hs, "c" + std::to_string(counter_cntrlr), m_pSocketComm);

			m_vDevices.push_back(temp);
			vr::VRServerDriverHost()->TrackedDeviceAdded(
				m_vDevices.back()->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller,
				temp);

			controller_hs = (controller_hs) ? 0 : 1;
			counter_cntrlr++;

		}
		else if (i == "t") {
			TrackerDriver* temp = new TrackerDriver("t" + std::to_string(counter_trkr), m_pSocketComm);

			m_vDevices.push_back(temp);
			vr::VRServerDriverHost()->TrackedDeviceAdded(
				m_vDevices.back()->GetSerialNumber().c_str(), vr::TrackedDeviceClass_GenericTracker,
				temp);

			counter_trkr++;

		}
		else {
			DriverLog("driver: unsupported device type: %s", i.c_str());
			return VRInitError_VendorSpecific_HmdFound_ConfigFailedSanityCheck;
		}
	}

	// start listening for device data
	m_pSocketComm->setCallback(this);

	// misc slow update thread
	m_bSlowUpdateThreadIsAlive = true;
	m_ptSlowUpdateThread = new std::thread(this->SlowUpdateThreadEnter, this);
	if (!m_bSlowUpdateThreadIsAlive) {
		DriverLog("driver: slow update thread broke on start\n");
		return VRInitError_IPC_Failed;
	}

	// settings manager
	m_pSettManTref = std::make_shared<HobovrTrackingRef_SettManager>("trsm0");
	vr::VRServerDriverHost()->TrackedDeviceAdded(
		m_pSettManTref->GetSerialNumber().c_str(), vr::TrackedDeviceClass_TrackingReference,
		m_pSettManTref.get());
	m_pSettManTref->UpdatePose();

	return VRInitError_None;
}
