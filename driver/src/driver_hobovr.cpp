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

#include "networking/zmqreqrep.h"

namespace hobovr {
	// le version
	static const uint32_t k_nHobovrVersionMajor = 1;
	static const uint32_t k_nHobovrVersionMinor = 0;
	static const uint32_t k_nHobovrVersionBuild = 0;
	static const std::string k_sHobovrVersionGG = "Multi Hobo Drifting";

} // namespace hobovr



/// <summary>
/// HMD Device Implementation
/// </summary>
class HeadsetDriver : public hobovr::HobovrDevice<false, false> {
public:
	HeadsetDriver(std::string myserial) :HobovrDevice(myserial, "hobovr_hmd_m") {

		m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_hmd_mh0";
		m_sBindPath = "{hobovr}/input/hobovr_hmd_profile.json";

		m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(
			k_pch_Hmd_Section, k_pch_Hmd_SecondsFromVsyncToPhotons_Float);

		m_flDisplayFrequency = vr::VRSettings()->GetFloat(
			k_pch_Hmd_Section, k_pch_Hmd_DisplayFrequency_Float);

		m_flIPD = vr::VRSettings()->GetFloat(k_pch_Hmd_Section,
			k_pch_Hmd_IPD_Float);

		m_fUserHead2EyeDepthMeters = vr::VRSettings()->GetFloat(k_pch_Hmd_Section,
			k_pch_Hmd_UserHead2EyeDepthMeters_Float);

		// log non boilerplate device specific settings 
		DriverLog("device hmd settings: vsync time %fs, display freq %f, ipd %fm, head2eye depth %fm", m_flSecondsFromVsyncToPhotons,
			m_flDisplayFrequency, m_flIPD, m_fUserHead2EyeDepthMeters);

		hobovr::HobovrComponent_t extDisplayComp = { hobovr::EHobovrCompType::EHobovrComp_ExtendedDisplay, vr::IVRDisplayComponent_Version };
		extDisplayComp.compHandle = std::make_shared<hobovr::HobovrExtendedDisplayComponent>();
		m_vComponents.push_back(extDisplayComp);
	}

	virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
		HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

		vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer,
			Prop_UserIpdMeters_Float, m_flIPD);
		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.f);
		vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer,
			Prop_DisplayFrequency_Float,
			m_flDisplayFrequency);
		vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer,
			Prop_SecondsFromVsyncToPhotons_Float,
			m_flSecondsFromVsyncToPhotons);

		// avoid "not fullscreen" warnings from vrmonitor
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
			Prop_IsOnDesktop_Bool, false);

		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
			Prop_DisplayDebugMode_Bool, false);

		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
			Prop_UserHeadToEyeDepthMeters_Float, m_fUserHead2EyeDepthMeters);

		return VRInitError_None;
	}

	void UpdateSectionSettings() {
		// get new ipd
		m_flIPD = vr::VRSettings()->GetFloat(k_pch_Hmd_Section,
			k_pch_Hmd_IPD_Float);
		// set new ipd
		vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer,
			Prop_UserIpdMeters_Float, m_flIPD);

		DriverLog("device hmd: ipd set to %f", m_flIPD);
	}


	void RunFrame(std::vector<float>& trackingPacket) {
		m_Pose.result = TrackingResult_Running_OK;
		m_Pose.vecPosition[0] = trackingPacket[0];
		m_Pose.vecPosition[1] = trackingPacket[1];
		m_Pose.vecPosition[2] = trackingPacket[2];

		m_Pose.qRotation =
			HmdQuaternion_Init(trackingPacket[3], trackingPacket[4],
				trackingPacket[5], trackingPacket[6]);

		m_Pose.vecVelocity[0] = trackingPacket[7];
		m_Pose.vecVelocity[1] = trackingPacket[8];
		m_Pose.vecVelocity[2] = trackingPacket[9];

		m_Pose.vecAngularVelocity[0] = trackingPacket[10];
		m_Pose.vecAngularVelocity[1] = trackingPacket[11];
		m_Pose.vecAngularVelocity[2] = trackingPacket[12];

		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
				m_unObjectId, m_Pose, sizeof(DriverPose_t));
		}
	}


private:
	float m_flSecondsFromVsyncToPhotons;
	float m_flDisplayFrequency;
	float m_flIPD;
	float m_fUserHead2EyeDepthMeters;
};
