// (c) 2021 Okawo
// This code is licensed under MIT license (see LICENSE for details)


//#include "openvr.h"
#include <openvr_driver.h>
//#include "openvr_capi.h"
#include "driverlog.h"

#include <chrono>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include "ref/receiver_win.h"

#elif defined(__linux__)
#include "ref/receiver_linux.h"
#define _stricmp strcasecmp

#endif

#if defined(_WINDOWS)
#include <windows.h>
#endif



#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <variant>

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

namespace hobovr {
  // le version
  static const uint32_t k_nHobovrVersionMajor = 0;
  static const uint32_t k_nHobovrVersionMinor = 6;
  static const uint32_t k_nHobovrVersionBuild = 6;
  static const std::string k_sHobovrVersionGG = "the hidden world";

} // namespace hobovr

// keys for use with the settings API
// driver keys
static const char *const k_pch_Hobovr_Section = "driver_hobovr";
static const char *const k_pch_Hobovr_UduDeviceManifestList_String = "uduSettings";

// hmd device keys
static const char *const k_pch_Hmd_Section = "hobovr_device_hmd";
static const char *const k_pch_Hmd_SecondsFromVsyncToPhotons_Float = "secondsFromVsyncToPhotons";
static const char *const k_pch_Hmd_DisplayFrequency_Float = "displayFrequency";
static const char* const k_pch_Hmd_IPD_Float = "IPD";
static const char* const k_pch_Hmd_UserHead2EyeDepthMeters_Float = "UserHeadToEyeDepthMeters";

// include has to be here, dont ask
#include "ref/hobovr_device_base.h"
#include "ref/hobovr_components.h"

//-----------------------------------------------------------------------------
// Purpose: hmd device implementation
//-----------------------------------------------------------------------------

class HeadsetDriver : public hobovr::HobovrDevice<false, false> {
public:
  	HeadsetDriver(std::string myserial):HobovrDevice(myserial, "hobovr_hmd_m") {

		m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_hmd_mh0";
		m_sBindPath = "{hobovr}/input/hobovr_hmd_profile.json";

		m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(
			k_pch_Hmd_Section,
			k_pch_Hmd_SecondsFromVsyncToPhotons_Float
		);

		m_flDisplayFrequency = vr::VRSettings()->GetFloat(
			k_pch_Hmd_Section,
			k_pch_Hmd_DisplayFrequency_Float
		);

		m_flIPD = vr::VRSettings()->GetFloat(
			k_pch_Hmd_Section,
			k_pch_Hmd_IPD_Float
		);

		m_fUserHead2EyeDepthMeters = vr::VRSettings()->GetFloat(
			k_pch_Hmd_Section,
			k_pch_Hmd_UserHead2EyeDepthMeters_Float
		);

		// log non boilerplate device specific settings 
		DriverLog("device hmd settings: vsync time %fs, display freq %f, ipd %fm, head2eye depth %fm",
			m_flSecondsFromVsyncToPhotons,
			m_flDisplayFrequency,
			m_flIPD,
			m_fUserHead2EyeDepthMeters
		);

		hobovr::HobovrComponent_t extDisplayComp;
		extDisplayComp.type = hobovr::EHobovrCompType::EHobovrComp_ExtendedDisplay;
		extDisplayComp.tag = vr::IVRDisplayComponent_Version;
		extDisplayComp.ptr_handle = new hobovr::HobovrExtendedDisplayComponent();
		m_vComponents.push_back(extDisplayComp);
  	}

  	EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
		HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_UserIpdMeters_Float,
			m_flIPD
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_UserHeadToEyeDepthMeters_Float,
			0.f
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_DisplayFrequency_Float,
			m_flDisplayFrequency
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_SecondsFromVsyncToPhotons_Float,
			m_flSecondsFromVsyncToPhotons
		);

		// avoid "not fullscreen" warnings from vrmonitor
		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			Prop_IsOnDesktop_Bool,
			false
		);

		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			Prop_DisplayDebugMode_Bool,
			false
		);

		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			Prop_UserHeadToEyeDepthMeters_Float,
			m_fUserHead2EyeDepthMeters
		);

		return VRInitError_None;
  	}

	void UpdateSectionSettings() {
		// get new ipd
		m_flIPD = vr::VRSettings()->GetFloat(
			k_pch_Hmd_Section,
			k_pch_Hmd_IPD_Float
		);
		// set new ipd
		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_UserIpdMeters_Float,
			m_flIPD
		);

		DriverLog("device hmd: ipd set to %f", m_flIPD);
	}


	void RunFrame(std::vector<float> &trackingPacket) override {
		DriverPose_t pose;
		pose.poseTimeOffset = m_fPoseTimeOffset;
		pose.result = TrackingResult_Running_OK;
		pose.poseIsValid = true;
		pose.deviceIsConnected = true;
		pose.vecPosition[0] = trackingPacket[0];
		pose.vecPosition[1] = trackingPacket[1];
		pose.vecPosition[2] = trackingPacket[2];

		pose.qRotation = {
			(double)trackingPacket[3],
			(double)trackingPacket[4],
			(double)trackingPacket[5],
			(double)trackingPacket[6]
		};

		pose.vecVelocity[0] = trackingPacket[7];
		pose.vecVelocity[1] = trackingPacket[8];
		pose.vecVelocity[2] = trackingPacket[9];

		pose.vecAngularVelocity[0] = trackingPacket[10];
		pose.vecAngularVelocity[1] = trackingPacket[11];
		pose.vecAngularVelocity[2] = trackingPacket[12];

		// pose.poseIsValid = true;
		// pose.result = TrackingResult_Running_OK;
		// pose.deviceIsConnected = true;

		// pose.qWorldFromDriverRotation = HmdQuaternion_Init( 1, 0, 0, 0 );
		// pose.qDriverFromHeadRotation = HmdQuaternion_Init( 1, 0, 0, 0 );

		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
				m_unObjectId,
				pose,
				sizeof(pose)
			);
		}
  	}


private:
	float m_flSecondsFromVsyncToPhotons;
	float m_flDisplayFrequency;
	float m_flIPD;
	float m_fUserHead2EyeDepthMeters;
};

//-----------------------------------------------------------------------------
// Purpose:controller device implementation
//-----------------------------------------------------------------------------

class ControllerDriver : public hobovr::HobovrDevice<true, false> {
public:
	ControllerDriver(
		bool side,
		std::string myserial,
		const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj
	): HobovrDevice(myserial, "hobovr_controller_m", ReceiverObj), m_bHandSide(side) {

		m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_controller_mc0";
		m_sBindPath = "{hobovr}/input/hobovr_controller_profile.json";

		m_skeletonHandle = vr::k_ulInvalidInputComponentHandle;
	}

	EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
		HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

		// the "make me look like a vive wand" bullshit
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_TrackingSystemName_String,
			"lighthouse"
		);
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_SerialNumber_String, m_serialNumber.c_str());
		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			vr::Prop_WillDriftInYaw_Bool,
			false
		);
		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			vr::Prop_DeviceIsWireless_Bool,
			true
		);
		// vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_DeviceIsCharging_Bool, false);
		// vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, vr::Prop_DeviceBatteryPercentage_Float, 1.f); // Always charged

		// why is this here? this is a controller!
		vr::HmdMatrix34_t l_matrix = {
			-1.f, 0.f, 0.f, 0.f,
			0.f, 0.f, -1.f, 0.f,
			0.f, -1.f, 0.f, 0.f
		};
		vr::VRProperties()->SetProperty(
			m_ulPropertyContainer,
			vr::Prop_StatusDisplayTransform_Matrix34,
			&l_matrix, sizeof(vr::HmdMatrix34_t),
			vr::k_unHmdMatrix34PropertyTag
		);

		// vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_UpdateAvailable_Bool, false);
		// vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_ManualUpdate_Bool, false);
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_Firmware_ManualUpdateURL_String, "https://developer.valvesoftware.com/wiki/SteamVR/HowTo_Update_Firmware");
		// vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_DeviceProvidesBatteryStatus_Bool, true);
		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			vr::Prop_DeviceCanPowerOff_Bool,
			true
		);
		vr::VRProperties()->SetInt32Property(
			m_ulPropertyContainer,
			vr::Prop_DeviceClass_Int32,
			vr::TrackedDeviceClass_Controller
		);
		// vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_ForceUpdateRequired_Bool, false);
		// vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Identifiable_Bool, true);
		// vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_RemindUpdate_Bool, false);
		vr::VRProperties()->SetInt32Property(
			m_ulPropertyContainer,
			vr::Prop_Axis0Type_Int32,
			vr::k_eControllerAxis_TrackPad
		);
		vr::VRProperties()->SetInt32Property(
			m_ulPropertyContainer,
			vr::Prop_Axis1Type_Int32,
			vr::k_eControllerAxis_Trigger
		);
		// vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_ControllerRoleHint_Int32, (m_bHandSide) ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand);
		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			vr::Prop_HasDisplayComponent_Bool,
			false
		);
		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			vr::Prop_HasCameraComponent_Bool,
			false
		);
		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			vr::Prop_HasDriverDirectModeComponent_Bool,
			false
		);
		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			vr::Prop_HasVirtualDisplayComponent_Bool,
			false
		);
		vr::VRProperties()->SetInt32Property(
			m_ulPropertyContainer,
			vr::Prop_ControllerHandSelectionPriority_Int32,
			0
		);
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_ModelNumber_String,
			"Vive. Controller MV"
		);
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_SerialNumber_String, m_serialNumber.c_str()); // Changed
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_RenderModelName_String, "vr_controller_vive_1_5");
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_ManufacturerName_String,
			"HTC"
		);
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_TrackingFirmwareVersion_String,
			"1533720215 htcvrsoftware@firmware-win32 2018-08-08 FPGA 262(1.6/0/0) BL 0 VRC 1533720214 Radio 1532585738"
		);
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_HardwareRevision_String,
			"product 129 rev 1.5.0 lot 2000/0/0 0"
		);
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_ConnectedWirelessDongle_String,
			"1E8092840E"
		); // Changed
		vr::VRProperties()->SetUint64Property(
			m_ulPropertyContainer,
			vr::Prop_HardwareRevision_Uint64,
			2164327680U
		);
		vr::VRProperties()->SetUint64Property(
			m_ulPropertyContainer,
			vr::Prop_FirmwareVersion_Uint64,
			1533720215U
		);
		vr::VRProperties()->SetUint64Property(
			m_ulPropertyContainer,
			vr::Prop_FPGAVersion_Uint64,
			262U
		);
		vr::VRProperties()->SetUint64Property(
			m_ulPropertyContainer,
			vr::Prop_VRCVersion_Uint64,
			1533720214U
		);
		vr::VRProperties()->SetUint64Property(
			m_ulPropertyContainer,
			vr::Prop_RadioVersion_Uint64,
			1532585738U
		);
		vr::VRProperties()->SetUint64Property(
			m_ulPropertyContainer,
			vr::Prop_DongleVersion_Uint64,
			1461100729U
		);
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_ResourceRoot_String,
			"htc"
		);
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_RegisteredDeviceType_String,
			(m_bHandSide) ? "htc/vive_controllerLHR-F94B3BD9" : "htc/vive_controllerLHR-F94B3BD8"
		); // Changed
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_InputProfilePath_String,
			"{htc}/input/vive_controller_profile.json"
		);
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceOff_String, "{htc}/icons/controller_status_off.png");
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearching_String, "{htc}/icons/controller_status_searching.gif");
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{htc}/icons/controller_status_searching_alert.gif");
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReady_String, "{htc}/icons/controller_status_ready.png");
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{htc}/icons/controller_status_ready_alert.png");
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceNotReady_String, "{htc}/icons/controller_status_error.png");
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceStandby_String, "{htc}/icons/controller_status_off.png");
		// vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceAlertLow_String, "{htc}/icons/controller_status_ready_low.png");
		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			vr::Prop_ControllerType_String,
			"vive_controller"
		);

		if (m_bHandSide) {
			vr::VRProperties()->SetInt32Property(
				m_ulPropertyContainer,
				Prop_ControllerRoleHint_Int32,
				TrackedControllerRole_RightHand
			);
		  // vr::VRDriverInput()->CreateSkeletonComponent(m_ulPropertyContainer, "/input/skeleton/right", 
		  //                 "/skeleton/hand/right", 
		  //                 "/pose/raw", vr::VRSkeletalTracking_Partial
		  //                 , nullptr, 0U, &m_skeletonHandle);
		} else {
			vr::VRProperties()->SetInt32Property(
				m_ulPropertyContainer,
				Prop_ControllerRoleHint_Int32,
				TrackedControllerRole_LeftHand
			);
			// vr::VRDriverInput()->CreateSkeletonComponent(m_ulPropertyContainer, "/input/skeleton/left", 
			//                 "/skeleton/hand/left", 
			//                 "/pose/raw", vr::VRSkeletalTracking_Partial
			//                 , nullptr, 0U, &m_skeletonHandle);
		}

		// create all the bool input components
		vr::VRDriverInput()->CreateBooleanComponent(
			m_ulPropertyContainer,
			"/input/grip/click",
			&m_compGrip
		);

		vr::VRDriverInput()->CreateBooleanComponent(
			m_ulPropertyContainer,
			"/input/system/click",
			&m_compSystem
		);

		vr::VRDriverInput()->CreateBooleanComponent(
			m_ulPropertyContainer,
			"/input/application_menu/click",
			&m_compAppMenu
		);

		vr::VRDriverInput()->CreateBooleanComponent(
			m_ulPropertyContainer,
			"/input/trackpad/click",
			&m_compTrackpadClick
		);

		vr::VRDriverInput()->CreateBooleanComponent(
			m_ulPropertyContainer,
			"/input/trackpad/touch",
			&m_compTrackpadTouch
		);

		vr::VRDriverInput()->CreateBooleanComponent(
			m_ulPropertyContainer,
			"/input/trigger/click",
			&m_compTriggerClick
		);


		// trigger
		vr::VRDriverInput()->CreateScalarComponent(
			m_ulPropertyContainer,
			"/input/trigger/value",
			&m_compTrigger,
			vr::VRScalarType_Absolute,
			vr::VRScalarUnits_NormalizedOneSided
		);

		// trackpad
		vr::VRDriverInput()->CreateScalarComponent(
			m_ulPropertyContainer,
			"/input/trackpad/x",
			&m_compTrackpadX,
			vr::VRScalarType_Absolute,
			vr::VRScalarUnits_NormalizedTwoSided
		);
		vr::VRDriverInput()->CreateScalarComponent(
			m_ulPropertyContainer,
			"/input/trackpad/y",
			&m_compTrackpadY,
			vr::VRScalarType_Absolute,
			vr::VRScalarUnits_NormalizedTwoSided
		);


		return VRInitError_None;
	}


	void RunFrame(std::vector<float> &lastRead) override {
		// update all the things
		DriverPose_t pose;
		pose.poseTimeOffset = m_fPoseTimeOffset;
		pose.result = TrackingResult_Running_OK;
		pose.poseIsValid = true;
		pose.deviceIsConnected = true;
		pose.vecPosition[0] = lastRead[0];
		pose.vecPosition[1] = lastRead[1];
		pose.vecPosition[2] = lastRead[2];

		pose.qRotation = {
			(double)lastRead[3],
			(double)lastRead[4],
			(double)lastRead[5],
			(double)lastRead[6]
		};

		pose.vecVelocity[0] = lastRead[7];
		pose.vecVelocity[1] = lastRead[8];
		pose.vecVelocity[2] = lastRead[9];

		pose.vecAngularVelocity[0] = lastRead[10];
		pose.vecAngularVelocity[1] = lastRead[11];
		pose.vecAngularVelocity[2] = lastRead[12];

		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
				m_unObjectId,
				pose,
				sizeof(pose)
			);
		}

		auto ivrinput_cache = vr::VRDriverInput();

		ivrinput_cache->UpdateBooleanComponent(
			m_compGrip,
			(bool)lastRead[13],
			(double)m_fPoseTimeOffset
		);

		ivrinput_cache->UpdateBooleanComponent(
			m_compSystem,
			(bool)lastRead[14],
			(double)m_fPoseTimeOffset
		);

		ivrinput_cache->UpdateBooleanComponent(
			m_compAppMenu,
			(bool)lastRead[15],
			(double)m_fPoseTimeOffset
		);

		ivrinput_cache->UpdateBooleanComponent(
			m_compTrackpadClick,
			(bool)lastRead[16],
			(double)m_fPoseTimeOffset
		);

		ivrinput_cache->UpdateScalarComponent(
			m_compTrigger,
			lastRead[17],
			(double)m_fPoseTimeOffset
		);

		ivrinput_cache->UpdateScalarComponent(
			m_compTrackpadX,
			lastRead[18],
			(double)m_fPoseTimeOffset
		);

		ivrinput_cache->UpdateScalarComponent(
			m_compTrackpadY,
			lastRead[19],
			(double)m_fPoseTimeOffset
		);


		ivrinput_cache->UpdateBooleanComponent(
			m_compTrackpadTouch,
			(bool)lastRead[20],
			(double)m_fPoseTimeOffset
		);

		ivrinput_cache->UpdateBooleanComponent(
			m_compTriggerClick,
			(bool)lastRead[21],
			(double)m_fPoseTimeOffset
		);

	}

private:
	vr::VRInputComponentHandle_t m_compGrip;
	vr::VRInputComponentHandle_t m_compSystem;
	vr::VRInputComponentHandle_t m_compAppMenu;
	vr::VRInputComponentHandle_t m_compTrigger;
	vr::VRInputComponentHandle_t m_compTrackpadX;
	vr::VRInputComponentHandle_t m_compTrackpadY;
	vr::VRInputComponentHandle_t m_compTrackpadTouch;
	vr::VRInputComponentHandle_t m_compTriggerClick;
	vr::VRInputComponentHandle_t m_compTrackpadClick;

	vr::VRInputComponentHandle_t m_skeletonHandle;

	bool m_bHandSide;

};


//-----------------------------------------------------------------------------
// Purpose: tracker device implementation
//-----------------------------------------------------------------------------
class TrackerDriver : public hobovr::HobovrDevice<true, false> {
public:
	TrackerDriver(
		std::string myserial,
		const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj
	): HobovrDevice(myserial, "hobovr_tracker_m", ReceiverObj) {

		m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_tracker_mt0";
		m_sBindPath = "{hobovr}/input/hobovr_tracker_profile.json";
	}

  EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
	HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

	vr::VRProperties()->SetInt32Property(
		m_ulPropertyContainer,
		Prop_ControllerRoleHint_Int32,
		TrackedControllerRole_RightHand
	);

	return VRInitError_None;
  }


	void RunFrame(std::vector<float> &lastRead) override {
		// update all the things
		DriverPose_t pose;
		pose.poseTimeOffset = m_fPoseTimeOffset;
		pose.result = TrackingResult_Running_OK;
		pose.poseIsValid = true;
		pose.deviceIsConnected = true;
		pose.vecPosition[0] = lastRead[0];
		pose.vecPosition[1] = lastRead[1];
		pose.vecPosition[2] = lastRead[2];

		pose.qRotation = {
			(double)lastRead[3],
			(double)lastRead[4],
			(double)lastRead[5],
			(double)lastRead[6]
		};

		pose.vecVelocity[0] = lastRead[7];
		pose.vecVelocity[1] = lastRead[8];
		pose.vecVelocity[2] = lastRead[9];

		pose.vecAngularVelocity[0] = lastRead[10];
		pose.vecAngularVelocity[1] = lastRead[11];
		pose.vecAngularVelocity[2] = lastRead[12];

		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
				m_unObjectId,
				pose,
				sizeof(pose)
			);
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: settings manager using a tracking reference, this is meant to be
// a settings and other utility communication and management tool
// for that purpose it will have a it's own server connection and poser protocol
// for now tho it will just stay as a tracking reference
// also a device that will always be present with hobo_vr devices from now on
// it will also allow live device management in the future
// 
// think of it as a settings manager made to look like a tracking reference
//-----------------------------------------------------------------------------

enum HobovrTrackingRef_Msg_type
{
	Emsg_invalid = 0,
	Emsg_ipd = 10,
	Emsg_uduString = 20,
	Emsg_poseTimeOffset = 30,
	Emsg_distortion = 40,
	Emsg_eyeGap = 50,
	Emsg_setSelfPose = 60,
};

enum HobovrVendorEvents
{
	UduChange = 19998, // in the vendor event range
};

std::vector<std::pair<std::string, int>> g_vpUduChangeBuffer;

class HobovrTrackingRef_SettManager: public vr::ITrackedDeviceServerDriver, public SockReceiver::Callback {
private:
  std::shared_ptr<SockReceiver::DriverReceiver> m_pSocketComm;
public:
	HobovrTrackingRef_SettManager(std::string myserial): m_sSerialNumber(myserial) {
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
		m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

		m_sModelNumber = "tracking_reference_" + m_sSerialNumber;
		m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_tracking_reference";

		DriverLog("device: settings manager tracking reference created\n");

		// manager stuff
		try {
			m_pSocketComm = std::make_shared<SockReceiver::DriverReceiver>("h520");
			m_pSocketComm->m_sIdMessage = "monky\n";
			m_pSocketComm->start();
		} catch (...) {
			DriverLog("tracking reference: couldn't connect to the server");
		}
		m_pSocketComm->setCallback(this);

	}

	void OnPacket(char* buff, int len) {

		if (len != 523) {
			return; // do nothing if bad message
		}

		uint32_t* data = (uint32_t*)buff;
		// DriverLog("tracking reference: message %d %d %d %d", data[0], data[1], data[2], data[129]);
		switch(data[0]) {
			case Emsg_ipd: {
				float newIpd = (float)data[1]/(float)data[2];
				vr::VRSettings()->SetFloat(
					k_pch_Hmd_Section,
					k_pch_Hmd_IPD_Float,
					newIpd
				);
				m_pSocketComm->send2("2000");
				DriverLog("tracking reference: ipd change request processed");
				break;
			}

			case Emsg_uduString: {
				int iUduLen = data[1];
				std::vector<std::pair<std::string, int>> temp;
				for (int i=0; i<iUduLen*2; i+=2) {
					int dt = data[2+i];
					int dp = data[2+i+1];
					std::pair<std::string, int> p;
					p.second = dp;

					if (dt == 0)
						p.first = "h";
					else if (dt == 1)
						p.first = "c";
					else if (dt == 2)
						p.first = "t";

					temp.push_back(p);
				}
				g_vpUduChangeBuffer = temp;

				vr::VREvent_Notification_t data = {20, 0};
				vr::VRServerDriverHost()->VendorSpecificEvent(
					m_unObjectId,
					(vr::EVREventType)HobovrVendorEvents::UduChange,
					(VREvent_Data_t&)data,
					0
				);

				DriverLog(
					"tracking reference: udu settings change request processed"
				);
				m_pSocketComm->send2("2000");
				break;
			}

			case Emsg_setSelfPose: {
				DriverPose_t pose;
				pose.poseTimeOffset = 0;
				pose.result = TrackingResult_Running_OK;
				pose.poseIsValid = true;
				pose.deviceIsConnected = true;
				pose.poseTimeOffset = 0;
				pose.qWorldFromDriverRotation = {1, 0, 0, 0};
				pose.qDriverFromHeadRotation = {1, 0, 0, 0};
				pose.qRotation = {1, 0, 0, 0};
				pose.vecPosition[0] = 0.01;
				pose.vecPosition[1] = 0;
				pose.vecPosition[2] = 0;
				pose.willDriftInYaw = false;
				pose.deviceIsConnected = true;
				pose.poseIsValid = true;
				pose.shouldApplyHeadModel = false;

				pose.vecPosition[0] = (float)data[1]/(float)data[2];
				pose.vecPosition[1] = (float)data[3]/(float)data[4];
				pose.vecPosition[2] = (float)data[5]/(float)data[6];
				if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
					vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
						m_unObjectId,
						pose,
						sizeof(pose)
					);
				}

				m_pSocketComm->send2("2000");
				DriverLog("tracking reference: pose change request processed");
				break;
			}

			case Emsg_distortion: {
				float newK1 = (float)data[1]/(float)data[2];
				float newK2 = (float)data[3]/(float)data[4];
				float newZoomW = (float)data[5]/(float)data[6];
				float newZoomH = (float)data[7]/(float)data[8];
				vr::VRSettings()->SetFloat(
					hobovr::k_pch_ExtDisplay_Section,
					hobovr::k_pch_ExtDisplay_DistortionK1_Float,
					newK1
				);

				vr::VRSettings()->SetFloat(
					hobovr::k_pch_ExtDisplay_Section,
					hobovr::k_pch_ExtDisplay_DistortionK2_Float,
					newK2
				);

				vr::VRSettings()->SetFloat(
					hobovr::k_pch_ExtDisplay_Section,
					hobovr::k_pch_ExtDisplay_ZoomWidth_Float,
					newZoomW
				);

				vr::VRSettings()->SetFloat(
					hobovr::k_pch_ExtDisplay_Section,
					hobovr::k_pch_ExtDisplay_ZoomHeight_Float,
					newZoomH
				);


				m_pSocketComm->send2("2000");
				DriverLog("tracking reference: distortion update request processed");
				break;
			}

			case Emsg_eyeGap: {
				vr::VRSettings()->SetInt32(
					hobovr::k_pch_ExtDisplay_Section,
					hobovr::k_pch_ExtDisplay_EyeGapOffset_Int,
					data[1]
				);

				m_pSocketComm->send2("2000");
				DriverLog("tracking reference: eye gap change request processed");
				break;
			}

			case Emsg_poseTimeOffset: {
				float newTimeOffset = (float)data[1]/(float)data[2];
				vr::VRSettings()->SetFloat(
					k_pch_Hobovr_Section,
					hobovr::k_pch_Hobovr_PoseTimeOffset_Float,
					newTimeOffset
				);

				m_pSocketComm->send2("2000");
				DriverLog(
					"tracking reference: pose time offset change request processed"
				);
				break;
			}

			default:
				DriverLog("tracking reference: message not recognized");
				m_pSocketComm->send2("-100");
		}
	}

	vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
		m_unObjectId = unObjectId;
		m_ulPropertyContainer =
			vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			Prop_ModelNumber_String,
			m_sModelNumber.c_str()
		);

		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			Prop_RenderModelName_String,
			m_sRenderModelPath.c_str()
		);

		DriverLog("device: tracking reference activated\n");
		DriverLog("device: \tserial: %s\n", m_sSerialNumber.c_str());
		DriverLog("device: \tmodel: %s\n", m_sModelNumber.c_str());
		DriverLog("device: \trender model path: \"%s\"\n", m_sRenderModelPath.c_str());

		// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
		vr::VRProperties()->SetUint64Property(
			m_ulPropertyContainer,
			Prop_CurrentUniverseId_Uint64,
			2
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_FieldOfViewLeftDegrees_Float,
			180
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_FieldOfViewRightDegrees_Float,
			180
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_FieldOfViewTopDegrees_Float,
			180
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_FieldOfViewBottomDegrees_Float,
			180
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_TrackingRangeMinimumMeters_Float,
			0
		);

		vr::VRProperties()->SetFloatProperty(
			m_ulPropertyContainer,
			Prop_TrackingRangeMaximumMeters_Float,
			10
		);

		vr::VRProperties()->SetStringProperty(
			m_ulPropertyContainer,
			Prop_ModeLabel_String,
			m_sModelNumber.c_str()
		);

		vr::VRProperties()->SetBoolProperty(
			m_ulPropertyContainer,
			Prop_CanWirelessIdentify_Bool,
			false
		);

		DriverPose_t pose;
		pose.poseTimeOffset = 0;
		pose.result = TrackingResult_Running_OK;
		pose.poseIsValid = true;
		pose.deviceIsConnected = true;
		pose.poseTimeOffset = 0;
		pose.qWorldFromDriverRotation = {1, 0, 0, 0};
		pose.qDriverFromHeadRotation = {1, 0, 0, 0};
		pose.qRotation = {1, 0, 0, 0};
		pose.vecPosition[0] = 0.01;
		pose.vecPosition[1] = 0;
		pose.vecPosition[2] = 0;
		pose.willDriftInYaw = false;
		pose.deviceIsConnected = true;
		pose.poseIsValid = true;
		pose.shouldApplyHeadModel = false;

		pose.vecPosition[0] = 0;
		pose.vecPosition[1] = 0;
		pose.vecPosition[2] = 0;

		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
				m_unObjectId,
				pose,
				sizeof(pose)
			);
		}

		return VRInitError_None;
	}

	virtual void Deactivate() {
		DriverLog("device: \"%s\" deactivated\n", m_sSerialNumber.c_str());
		// "signal" device disconnected
		m_Pose.poseIsValid = false;
		m_Pose.deviceIsConnected = false;

		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
				m_unObjectId,
				m_Pose,
				sizeof(DriverPose_t)
			);
		}

		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	}

	void EnterStandby() {}

	void *GetComponent(const char *pchComponentNameAndVersion) {
		DriverLog("tracking reference: got a request for \"%s\" component, request ignored\n",
			pchComponentNameAndVersion
		);

		return NULL;
	}

	void DebugRequest(
		const char *pchRequest,
		char *pchResponseBuffer,
		uint32_t unResponseBufferSize
	) {

		DriverLog("device: \"%s\" got a debug request: \"%s\"",
			m_sSerialNumber.c_str(),
			pchRequest
		);

		if (unResponseBufferSize >= 1)
			pchResponseBuffer[0] = 0;
	}

	vr::DriverPose_t GetPose() {return m_Pose;}

	std::string GetSerialNumber() const { return m_sSerialNumber;}

	void UpdatePose() {
		if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
			DriverLog("tracking reference: pose updated");
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
				m_unObjectId,
				m_Pose,
				sizeof(DriverPose_t)
			);
		}
	}

private:
  // openvr api stuff
  vr::TrackedDeviceIndex_t m_unObjectId; // DO NOT TOUCH THIS, parent will handle this, use it as read only!
  vr::PropertyContainerHandle_t m_ulPropertyContainer; // THIS EITHER, use it as read only!

  std::string m_sRenderModelPath; // path to the device's render model, should be populated in the constructor of the derived class

  vr::DriverPose_t m_Pose; // device's pose, use this at runtime

  std::string m_sSerialNumber; // steamvr uses this to identify devices, no need for you to touch this after init
  std::string m_sModelNumber; // steamvr uses this to identify devices, no need for you to touch this after init

};


//-----------------------------------------------------------------------------
// Purpose: serverDriver
//-----------------------------------------------------------------------------

enum EHobovrDeviceNodeTypes {
	hmd = 0,
	controller = 1,
	tracker = 2
};


struct HobovrDeviceStorageNode_t {
	EHobovrDeviceNodeTypes type;
	void* handle;
};


class CServerDriver_hobovr : public IServerTrackedDeviceProvider, public SockReceiver::Callback {
public:
	CServerDriver_hobovr() {}
	virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();
	virtual const char *const *GetInterfaceVersions() {
		return vr::k_InterfaceVersions;
	}

	virtual bool ShouldBlockStandbyMode() { return false; }

	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}
	virtual void RunFrame();
	void OnPacket(char* buff, int len);

private:
	void SlowUpdateThread();
	static void SlowUpdateThreadEnter(CServerDriver_hobovr *ptr) {
		ptr->SlowUpdateThread();
	}

	void UpdateServerDeviceList();

	std::vector<HobovrDeviceStorageNode_t> m_vDevices;
	std::vector<HobovrDeviceStorageNode_t> m_vStandbyDevices;

	std::shared_ptr<SockReceiver::DriverReceiver> m_pSocketComm;
	std::shared_ptr<HobovrTrackingRef_SettManager> m_pSettManTref;

	bool m_bDeviceListSyncEvent = false;


	// slower thread stuff
	bool m_bSlowUpdateThreadIsAlive;
	std::thread* m_ptSlowUpdateThread;
};

// yes
EVRInitError CServerDriver_hobovr::Init(vr::IVRDriverContext *pDriverContext) {
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());

	DriverLog("driver: version: %d.%d.%d %s \n",
		hobovr::k_nHobovrVersionMajor,
		hobovr::k_nHobovrVersionMinor,
		hobovr::k_nHobovrVersionBuild,
		hobovr::k_sHobovrVersionGG.c_str()
	);

	std::string uduThing;
	char buf[1024];
	vr::VRSettings()->GetString(
		k_pch_Hobovr_Section,
		k_pch_Hobovr_UduDeviceManifestList_String,
		buf,
		sizeof(buf)
	);
	uduThing = buf;
	DriverLog("driver: udu settings: '%s'\n", uduThing.c_str());

	// udu setting parse is done by SockReceiver
	try{
		m_pSocketComm = std::make_shared<SockReceiver::DriverReceiver>(uduThing);
		m_pSocketComm->start();

	} catch (...){
		DriverLog("m_pSocketComm broke on create or broke on start, either way you're fucked\n");
		DriverLog("check if the server is running...\n");
		DriverLog("... 10061 means \"couldn't connect to server\"...(^_^;)..\n");
		return VRInitError_Init_WebServerFailed;

	}

	int counter_hmd = 0;
	int counter_cntrlr = 0;
	int counter_trkr = 0;
	int controller_hs = 1;

	// add new devices based on the udu parse 
	for (std::string i:m_pSocketComm->m_vsDevice_list) {
		if (i == "h") {
			HeadsetDriver* temp = new HeadsetDriver(
				"h" + std::to_string(counter_hmd)
			);

			vr::VRServerDriverHost()->TrackedDeviceAdded(
				temp->GetSerialNumber().c_str(),
				vr::TrackedDeviceClass_HMD,
				temp
			);

			m_vDevices.push_back({EHobovrDeviceNodeTypes::hmd, temp});

		  counter_hmd++;

		} else if (i == "c") {
			ControllerDriver* temp = new ControllerDriver(
				controller_hs,
				"c" + std::to_string(counter_cntrlr),
				m_pSocketComm
			);

			vr::VRServerDriverHost()->TrackedDeviceAdded(
						temp->GetSerialNumber().c_str(),
						vr::TrackedDeviceClass_Controller,
						temp);
			m_vDevices.push_back({EHobovrDeviceNodeTypes::controller, temp});

			controller_hs = (controller_hs) ? 0 : 1;
			counter_cntrlr++;

		} else if (i == "t") {
			TrackerDriver* temp = new TrackerDriver(
				"t" + std::to_string(counter_trkr),
				m_pSocketComm
			);

			vr::VRServerDriverHost()->TrackedDeviceAdded(
				temp->GetSerialNumber().c_str(),
				vr::TrackedDeviceClass_GenericTracker,
				temp
			);

			m_vDevices.push_back({EHobovrDeviceNodeTypes::tracker, temp});

			counter_trkr++;

		} else {
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
		m_pSettManTref->GetSerialNumber().c_str(),
		vr::TrackedDeviceClass_TrackingReference,
		m_pSettManTref.get()
	);

	m_pSettManTref->UpdatePose();

	return VRInitError_None;
}

void CServerDriver_hobovr::Cleanup() {
	DriverLog("driver cleanup called");
	m_pSocketComm->stop();
	m_bSlowUpdateThreadIsAlive = false;
	m_ptSlowUpdateThread->join();

	for (auto& i : m_vDevices) {
		switch (i.type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)i.handle;
				free(device);
				break;
			}
		}
	}

	for (auto& i : m_vStandbyDevices) {
		switch (i.type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)i.handle;
				free(device);
				break;
			}
		}
	}

	m_vDevices.clear();
	m_vStandbyDevices.clear();

	CleanupDriverLog();
	VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void CServerDriver_hobovr::OnPacket(char* buff, int len) {
  if (len == (m_pSocketComm->m_iExpectedMessageSize*4+3) && !m_bDeviceListSyncEvent)
  {
	float* temp= (float*)buff;
	std::vector<float> v(temp, temp+m_pSocketComm->m_iExpectedMessageSize);
	auto tempPose = SockReceiver::split_pk(v, m_pSocketComm->m_viEps);

	for (size_t i=0; i < m_vDevices.size(); i++){
		switch (m_vDevices[i].type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)m_vDevices[i].handle;
				device->RunFrame(tempPose[i]);
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)m_vDevices[i].handle;
				device->RunFrame(tempPose[i]);
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)m_vDevices[i].handle;
				device->RunFrame(tempPose[i]);
				break;
			}
		}

	}

  } else {
	DriverLog("driver: bad packet, expected %d, got %d. double check your udu settings\n", (m_pSocketComm->m_iExpectedMessageSize*4+3), len);
  }


}

void CServerDriver_hobovr::RunFrame() {
	vr::VREvent_t vrEvent;
	while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
		for (auto& i : m_vDevices) {
			switch (i.type) {
				case EHobovrDeviceNodeTypes::hmd: {
					HeadsetDriver* device = (HeadsetDriver*)i.handle;
					device->ProcessEvent(vrEvent);
					break;
				}

				case EHobovrDeviceNodeTypes::controller: {
					ControllerDriver* device = (ControllerDriver*)i.handle;
					device->ProcessEvent(vrEvent);
					break;
				}

				case EHobovrDeviceNodeTypes::tracker: {
					TrackerDriver* device = (TrackerDriver*)i.handle;
					device->ProcessEvent(vrEvent);
					break;
				}
			}
		}

		if (vrEvent.eventType == HobovrVendorEvents::UduChange) {
			DriverLog("udu change event");
			std::vector<std::string> newD;
			std::vector<int> newEps;
			auto uduBufferCopy = g_vpUduChangeBuffer;

			for (auto i : uduBufferCopy) {
				DriverLog("pair: (%c, %d)", i.first, i.second);
				newD.push_back(i.first);
				newEps.push_back(i.second);
			}

			m_bDeviceListSyncEvent = true;
			m_pSocketComm->UpdateParams(newD, newEps);
			UpdateServerDeviceList();
			m_bDeviceListSyncEvent = false;
		}
	}
}

void CServerDriver_hobovr::UpdateServerDeviceList() {
	for (auto i : m_vDevices) {
		switch (i.type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)i.handle;
				device->PowerOff();
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)i.handle;
				device->PowerOff();
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)i.handle;
				device->PowerOff();
				break;
			}
		}
		m_vStandbyDevices.push_back(i);
	}

	m_vDevices.clear();

	auto uduBufferCopy = g_vpUduChangeBuffer;

	int counter_hmd = 0;
	int counter_cntrlr = 0;
	int counter_trkr = 0;
	int controller_hs = 1;

	for (auto i : uduBufferCopy) {
		if (i.first == "h") {
			auto target = "h" + std::to_string(counter_hmd);
			auto key = [target](HobovrDeviceStorageNode_t d)->bool {
				switch (d.type) {
					case EHobovrDeviceNodeTypes::hmd:
						return ((HeadsetDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::controller:
						return ((ControllerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::tracker:
						return ((TrackerDriver*)d.handle)->GetSerialNumber() == target;
				}
				return false;
			};

			auto res = std::find_if(m_vStandbyDevices.begin(), m_vStandbyDevices.end(), key);
			if (res != m_vStandbyDevices.end()) {
				((HeadsetDriver*)((*res).handle))->PowerOn();
				m_vDevices.push_back(*res);
				m_vStandbyDevices.erase(res);
			} else {
				HeadsetDriver* temp = new HeadsetDriver(
					"h" + std::to_string(counter_hmd)
				);

				vr::VRServerDriverHost()->TrackedDeviceAdded(
					temp->GetSerialNumber().c_str(),
					vr::TrackedDeviceClass_HMD,
					temp
				);
				m_vDevices.push_back({EHobovrDeviceNodeTypes::hmd, temp});
			}

			counter_hmd++;
		} else if (i.first == "c") {
			auto target = "c" + std::to_string(counter_cntrlr);
			auto key = [target](HobovrDeviceStorageNode_t d)->bool {
				switch (d.type) {
					case EHobovrDeviceNodeTypes::hmd:
						return ((HeadsetDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::controller:
						return ((ControllerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::tracker:
						return ((TrackerDriver*)d.handle)->GetSerialNumber() == target;
				}
				return false;
			};

			auto res = std::find_if(m_vStandbyDevices.begin(), m_vStandbyDevices.end(), key);
			if (res != m_vStandbyDevices.end()) {
				((ControllerDriver*)((*res).handle))->PowerOn();
				m_vDevices.push_back(*res);
				m_vStandbyDevices.erase(res);
			} else {
				ControllerDriver* temp = new ControllerDriver(
					controller_hs,
					"c" + std::to_string(counter_cntrlr),
					m_pSocketComm
				);

				vr::VRServerDriverHost()->TrackedDeviceAdded(
					temp->GetSerialNumber().c_str(),
					vr::TrackedDeviceClass_Controller,
					temp
				);
				m_vDevices.push_back({EHobovrDeviceNodeTypes::controller, temp});

			}

			controller_hs = (controller_hs) ? 0 : 1;
			counter_cntrlr++;

		} else if (i.first == "t") {
			auto target = "t" + std::to_string(counter_trkr);
			auto key = [target](HobovrDeviceStorageNode_t d)->bool {
				switch (d.type) {
					case EHobovrDeviceNodeTypes::hmd:
						return ((HeadsetDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::controller:
						return ((ControllerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::tracker:
						return ((TrackerDriver*)d.handle)->GetSerialNumber() == target;
				}
				return false;
			};

			auto res = std::find_if(m_vStandbyDevices.begin(), m_vStandbyDevices.end(), key);
			if (res != m_vStandbyDevices.end()) {
				((TrackerDriver*)((*res).handle))->PowerOn();
				m_vDevices.push_back(*res);
				m_vStandbyDevices.erase(res);
			} else {
				TrackerDriver* temp = new TrackerDriver("t" + std::to_string(counter_trkr), m_pSocketComm);

				vr::VRServerDriverHost()->TrackedDeviceAdded(
					temp->GetSerialNumber().c_str(),
					vr::TrackedDeviceClass_GenericTracker,
					temp
				);
				m_vDevices.push_back({EHobovrDeviceNodeTypes::tracker, temp});

			}

			counter_trkr++;
		}
	}

	g_vpUduChangeBuffer.clear();
}

void CServerDriver_hobovr::SlowUpdateThread() {
	DriverLog("driver: slow update thread started\n");
	int h = 0;
	while (m_bSlowUpdateThreadIsAlive){
		for (auto &i : m_vDevices){
			switch (i.type) {
				case EHobovrDeviceNodeTypes::hmd: {
					HeadsetDriver* device = (HeadsetDriver*)i.handle;
					device->UpdateDeviceBatteryCharge();
					device->CheckForUpdates();
					break;
				}

				case EHobovrDeviceNodeTypes::controller: {
					ControllerDriver* device = (ControllerDriver*)i.handle;
					device->UpdateDeviceBatteryCharge();
					device->CheckForUpdates();
					break;
				}

				case EHobovrDeviceNodeTypes::tracker: {
					TrackerDriver* device = (TrackerDriver*)i.handle;
					device->UpdateDeviceBatteryCharge();
					device->CheckForUpdates();
					break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::seconds(5));

		if (!h) {
			m_pSettManTref->UpdatePose();
			h = 1;
		}
	}
	DriverLog("driver: slow update thread closed\n");
	m_bSlowUpdateThreadIsAlive = false;

}

CServerDriver_hobovr g_hobovrServerDriver;

//-----------------------------------------------------------------------------
// Purpose: driverFactory
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void *HmdDriverFactory(
	const char *pInterfaceName,
	int *pReturnCode
) {

	if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName)) {
		return &g_hobovrServerDriver;
	}

	if (pReturnCode)
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

  return NULL;
}
