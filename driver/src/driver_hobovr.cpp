//============ Copyright (c) okawo, All rights reserved.
//============

//#include "openvr.h"
#include "openvr_driver.h"
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
  static const uint32_t k_nHobovrVersionMinor = 5;
  static const uint32_t k_nHobovrVersionBuild = 4;
  static const std::string k_sHobovrVersionGG = "phantom pain";

} // namespace hobovr

inline HmdQuaternion_t HmdQuaternion_Init(double w, double x, double y,
                                          double z) {
  HmdQuaternion_t quat;
  quat.w = w;
  quat.x = x;
  quat.y = y;
  quat.z = z;
  return quat;
}

// keys for use with the settings API
// driver keys
static const char *const k_pch_Hobovr_Section = "driver_hobovr";
static const char *const k_pch_Hobovr_UduDeviceManifestList_String = "DeviceManifestList";

// hmd device keys
static const char *const k_pch_Hmd_Section = "hobovr_device_hmd";
static const char *const k_pch_Hmd_SecondsFromVsyncToPhotons_Float = "secondsFromVsyncToPhotons";
static const char *const k_pch_Hmd_DisplayFrequency_Float = "displayFrequency";
static const char* const k_pch_Hmd_IPD_Float = "IPD";

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
        k_pch_Hmd_Section, k_pch_Hmd_SecondsFromVsyncToPhotons_Float);

    m_flDisplayFrequency = vr::VRSettings()->GetFloat(
        k_pch_Hmd_Section, k_pch_Hmd_DisplayFrequency_Float);

    m_flIPD = vr::VRSettings()->GetFloat(k_pch_Hmd_Section,
                                         k_pch_Hmd_IPD_Float);

    // log non boilerplate device specific settings 
    DriverLog("device hmd settings: vsync time %fs, display freq %f, ipd %fm", m_flSecondsFromVsyncToPhotons,
                    m_flDisplayFrequency, m_flIPD);

    hobovr::HobovrComponent_t extDisplayComp = {hobovr::THobovrCompType::THobovrComp_ExtendedDisplay, vr::IVRDisplayComponent_Version};
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

    return VRInitError_None;
  }


  void RunFrame(std::vector<float> &trackingPacket) {
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
};

//-----------------------------------------------------------------------------
// Purpose:controller device implementation
//-----------------------------------------------------------------------------
class ControllerDriver : public hobovr::HobovrDevice<true, false> {
public:
  ControllerDriver(bool side, std::string myserial, const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj):
  HobovrDevice(myserial, "hobovr_controller_m", ReceiverObj), m_bHandSide(side) {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_controller_mc0";
    m_sBindPath = "{hobovr}/input/hobovr_controller_profile.json";
  }

  virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
    HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff


    if (m_bHandSide) {
      vr::VRProperties()->SetInt32Property(m_ulPropertyContainer,
                                           Prop_ControllerRoleHint_Int32,
                                           TrackedControllerRole_RightHand);
    } else {
      vr::VRProperties()->SetInt32Property(m_ulPropertyContainer,
                                           Prop_ControllerRoleHint_Int32,
                                           TrackedControllerRole_LeftHand);
    }

    // create all the bool input components
    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer, "/input/grip/click", &m_compGrip);
    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer, "/input/system/click", &m_compSystem);
    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer, "/input/application_menu/click", &m_compAppMenu);
    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer, "/input/trackpad/click", &m_compTrackpadClick);
    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer, "/input/trackpad/touch", &m_compTrackpadTouch);
    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer, "/input/trigger/click", &m_compTriggerClick);

    // trigger
    vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer, "/input/trigger/value", &m_compTrigger,
        vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedOneSided);

    // trackpad
    vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer, "/input/trackpad/x", &m_compTrackpadX,
        vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
    vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer, "/input/trackpad/y", &m_compTrackpadY,
        vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);


    return VRInitError_None;
  }


  void RunFrame(std::vector<float> &lastRead) {
    // update all the things

    m_Pose.result = TrackingResult_Running_OK;

    m_Pose.vecPosition[0] = lastRead[0];
    m_Pose.vecPosition[1] = lastRead[1];
    m_Pose.vecPosition[2] = lastRead[2];

    m_Pose.qRotation =
        HmdQuaternion_Init(lastRead[3],
                           lastRead[4],
                           lastRead[5],
                           lastRead[6]);

    m_Pose.vecVelocity[0] = lastRead[7];
    m_Pose.vecVelocity[1] = lastRead[8];
    m_Pose.vecVelocity[2] = lastRead[9];

    m_Pose.vecAngularVelocity[0] =
        lastRead[10];
    m_Pose.vecAngularVelocity[1] =
        lastRead[11];
    m_Pose.vecAngularVelocity[2] =
        lastRead[12];

    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compGrip, (bool)lastRead[13], (double)m_fPoseTimeOffset);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compSystem, (bool)lastRead[14], (double)m_fPoseTimeOffset);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compAppMenu, (bool)lastRead[15], (double)m_fPoseTimeOffset);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTrackpadClick, (bool)lastRead[16],
        (double)m_fPoseTimeOffset);

    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrigger, lastRead[17], (double)m_fPoseTimeOffset);
    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrackpadX, lastRead[18], (double)m_fPoseTimeOffset);
    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrackpadY, lastRead[19], (double)m_fPoseTimeOffset);

    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTrackpadTouch, (bool)lastRead[20],
        (double)m_fPoseTimeOffset);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTriggerClick, (bool)lastRead[21],
        (double)m_fPoseTimeOffset);

    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
      vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
          m_unObjectId, m_Pose, sizeof(DriverPose_t));
    }
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

  bool m_bHandSide;
};


//-----------------------------------------------------------------------------
// Purpose: tracker device implementation
//-----------------------------------------------------------------------------
class TrackerDriver : public hobovr::HobovrDevice<true, false> {
public:
  TrackerDriver(std::string myserial, const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj):
  HobovrDevice(myserial, "hobovr_tracker_m", ReceiverObj) {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_tracker_mt0";
    m_sBindPath = "{hobovr}/input/hobovr_tracker_profile.json";
  }

  virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
    HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

    vr::VRProperties()->SetInt32Property(m_ulPropertyContainer,
                                         Prop_ControllerRoleHint_Int32,
                                         TrackedControllerRole_RightHand);

    return VRInitError_None;
  }


  void RunFrame(std::vector<float> &lastRead) {
    // update all the things

    m_Pose.result = TrackingResult_Running_OK;

    m_Pose.vecPosition[0] = lastRead[0];
    m_Pose.vecPosition[1] = lastRead[1];
    m_Pose.vecPosition[2] = lastRead[2];

    m_Pose.qRotation =
        HmdQuaternion_Init(lastRead[3],
                           lastRead[4],
                           lastRead[5],
                           lastRead[6]);

    m_Pose.vecVelocity[0] = lastRead[7];
    m_Pose.vecVelocity[1] = lastRead[8];
    m_Pose.vecVelocity[2] = lastRead[9];

    m_Pose.vecAngularVelocity[0] =
        lastRead[10];
    m_Pose.vecAngularVelocity[1] =
        lastRead[11];
    m_Pose.vecAngularVelocity[2] =
        lastRead[12];

    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
      vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
          m_unObjectId, m_Pose, sizeof(DriverPose_t));
    }
  }
};

//-----------------------------------------------------------------------------
// Purpose: serverDriver
//-----------------------------------------------------------------------------

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

  std::vector<hobovr::HobovrDeviceElement*> m_vDevices;

  std::shared_ptr<SockReceiver::DriverReceiver> m_pSocketComm;

  int m_iCallBack_packet_size;

  // slower thread stuff
  bool m_bSlowUpdateThreadIsAlive;
  std::thread* m_ptSlowUpdateThread;
};

EVRInitError CServerDriver_hobovr::Init(vr::IVRDriverContext *pDriverContext) {
  VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
  InitDriverLog(vr::VRDriverLog());

  DriverLog("driver version: %d.%d.%d %s \n", hobovr::k_nHobovrVersionMajor,
                            hobovr::k_nHobovrVersionMinor,
                            hobovr::k_nHobovrVersionBuild,
                            hobovr::k_sHobovrVersionGG);

  std::string uduThing;
  char buf[1024];
  vr::VRSettings()->GetString(k_pch_Hobovr_Section,
                              k_pch_Hobovr_UduDeviceManifestList_String, buf,
                              sizeof(buf));
  uduThing = buf;
  DriverLog("device manifest list: '%s'\n", uduThing.c_str());

  try{
    m_pSocketComm = std::make_shared<SockReceiver::DriverReceiver>(uduThing);
    m_iCallBack_packet_size = m_pSocketComm->m_iBuffSize;
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

  for (std::string i:m_pSocketComm->device_list) {
    if (i == "h") {
      HeadsetDriver* temp = new HeadsetDriver("h" + std::to_string(counter_hmd));

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back()->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD,
                    temp);

      counter_hmd++;

    } else if (i == "c") {
      ControllerDriver* temp = new ControllerDriver(controller_hs, "c" + std::to_string(counter_cntrlr), m_pSocketComm);

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back()->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller,
                    temp);

      controller_hs = (controller_hs) ? 0 : 1;
      counter_cntrlr++;

    } else if (i == "t") {
      TrackerDriver* temp = new TrackerDriver("t" + std::to_string(counter_trkr), m_pSocketComm);

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back()->GetSerialNumber().c_str(), vr::TrackedDeviceClass_GenericTracker,
                    temp);

      counter_trkr++;

    } else {
      DriverLog("unsupported device type: %s", i);
      return VRInitError_VendorSpecific_HmdFound_ConfigFailedSanityCheck;
    }
  }

  m_pSocketComm->setCallback(this);

  m_bSlowUpdateThreadIsAlive = true;
  m_ptSlowUpdateThread = new std::thread(this->SlowUpdateThreadEnter, this);
  if (!m_bSlowUpdateThreadIsAlive) {
    DriverLog("slow update thread broke on start\n");
    return VRInitError_Driver_RuntimeOutOfDate;
  }

  return VRInitError_None;
}

void CServerDriver_hobovr::Cleanup() {
  m_pSocketComm->stop();
  m_bSlowUpdateThreadIsAlive = false;
  m_ptSlowUpdateThread->join();

  for (auto& i : m_vDevices)
    delete i; 

  m_vDevices.clear();

  CleanupDriverLog();
  VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void CServerDriver_hobovr::OnPacket(char* buff, int len) {
  float* temp= (float*)buff;
  std::vector<float> v(temp, temp+m_iCallBack_packet_size);
  auto tempPose = SockReceiver::split_pk(v, m_pSocketComm->eps);

  if (len == (m_iCallBack_packet_size*4+3))
  {
    for (int i=0; i<m_vDevices.size(); i++){

      m_vDevices[i]->RunFrame(tempPose[i]);

    }

  } else {
    DriverLog("bad packet, expected %d, got %d, double check your udu settings\n", (m_iCallBack_packet_size*4+3), len);
  }


}

void CServerDriver_hobovr::RunFrame() {
  vr::VREvent_t vrEvent;
  while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
    for (auto &i : m_vDevices){

      i->ProcessEvent(vrEvent);

    }
  }
}

void CServerDriver_hobovr::SlowUpdateThread() {
  DriverLog("slow update thread started\n");
  while (m_bSlowUpdateThreadIsAlive){
    for (auto &i : m_vDevices)
      i->UpdateDeviceBatteryCharge();

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
  DriverLog("slow update thread closed\n");
  m_bSlowUpdateThreadIsAlive = false;

}

CServerDriver_hobovr g_hobovrServerDriver;

//-----------------------------------------------------------------------------
// Purpose: driverFactory
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void *HmdDriverFactory(const char *pInterfaceName,
                                      int *pReturnCode) {
  if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName)) {
    return &g_hobovrServerDriver;
  }

  if (pReturnCode)
    *pReturnCode = VRInitError_Init_InterfaceNotFound;

  return NULL;
}
