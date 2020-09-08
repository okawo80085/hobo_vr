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
static const char *const k_pch_Hobovr_Section = "driver_hobovr";
static const char *const k_pch_Hobovr_WindowX_Int32 = "windowX";
static const char *const k_pch_Hobovr_WindowY_Int32 = "windowY";
static const char *const k_pch_Hobovr_WindowWidth_Int32 = "windowWidth";
static const char *const k_pch_Hobovr_WindowHeight_Int32 = "windowHeight";
static const char *const k_pch_Hobovr_RenderWidth_Int32 = "renderWidth";
static const char *const k_pch_Hobovr_RenderHeight_Int32 = "renderHeight";
static const char *const k_pch_Hobovr_SecondsFromVsyncToPhotons_Float =
    "secondsFromVsyncToPhotons";
static const char *const k_pch_Hobovr_DisplayFrequency_Float =
    "displayFrequency";

static const char *const k_pch_Hobovr_DistortionK1_Float = "DistortionK1";
static const char *const k_pch_Hobovr_DistortionK2_Float = "DistortionK2";
static const char *const k_pch_Hobovr_ZoomWidth_Float = "ZoomWidth";
static const char *const k_pch_Hobovr_ZoomHeight_Float = "ZoomHeight";
static const char *const k_pch_Hobovr_UduDeviceManifestList_String = "DeviceManifestList";
static const char *const k_pch_Hobovr_IPD_Float = "IPD";

// include has to be here, dont ask
#include "ref/hobovr_device_base.h"
#include "ref/hobovr_components.h"

//-----------------------------------------------------------------------------
// Purpose: hmd device implementation
//-----------------------------------------------------------------------------

class HeadsetDriver : public hobovr::HobovrDevice<false> {
public:
  HeadsetDriver(std::string myserial):HobovrDevice(myserial, "hobovr_hmd_m") {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_hmd_mh0";
    m_sBindPath = "{hobovr}/input/hobovr_hmd_profile.json";

    m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(
        k_pch_Hobovr_Section, k_pch_Hobovr_SecondsFromVsyncToPhotons_Float);

    m_flDisplayFrequency = vr::VRSettings()->GetFloat(
        k_pch_Hobovr_Section, k_pch_Hobovr_DisplayFrequency_Float);

    m_flIPD = vr::VRSettings()->GetFloat(k_pch_Hobovr_Section,
                                         k_pch_Hobovr_IPD_Float);

    hobovr::HobovrComponent extDisplayComp;
    extDisplayComp.componentHandle = std::make_shared<hobovr::HobovrExtendedDisplayComponent>();
    extDisplayComp.componentNameAndVersion = extDisplayComp.componentHandle->GetComponentNameAndVersion();
    m_vComponents.push_back(extDisplayComp);

    m_Pose.poseTimeOffset = 0;
    m_Pose.poseIsValid = true;
    m_Pose.deviceIsConnected = true;
    m_Pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
    m_Pose.vecWorldFromDriverTranslation[0] = 0.;
    m_Pose.vecWorldFromDriverTranslation[1] = 0.;
    m_Pose.vecWorldFromDriverTranslation[2] = 0.;
    m_Pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
    m_Pose.vecDriverFromHeadTranslation[0] = 0.;
    m_Pose.vecDriverFromHeadTranslation[1] = 0.;
    m_Pose.vecDriverFromHeadTranslation[2] = 0.;
    m_Pose.vecPosition[0] = 0.;
    m_Pose.vecPosition[1] = 0.;
    m_Pose.vecPosition[2] = 0.;
    m_Pose.willDriftInYaw = true;
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

    return VRInitError_None;
  }

  template <typename Number>
  void RunFrame(std::vector<Number> &trackingPacket) {
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
class ControllerDriver : public hobovr::HobovrDevice<true> {
public:
  ControllerDriver(bool side, std::string myserial, const std::shared_ptr<SockReceiver::DriverReceiver> remotePoser):
  HobovrDevice(myserial, "hobovr_controller_m", remotePoser), m_bHandSide(side) {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_controller_mc0";
    m_sBindPath = "{hobovr}/input/hobovr_controller_profile.json";

    m_Pose.poseTimeOffset = 0;
    m_Pose.poseIsValid = true;
    m_Pose.deviceIsConnected = true;
    m_Pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
    m_Pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
    m_Pose.vecPosition[0] = 0.;
    if (side) {
      m_Pose.vecPosition[1] = 1.;
    } else {
      m_Pose.vecPosition[1] = 1.1;
    }

    m_Pose.vecPosition[2] = 0.;
    m_Pose.willDriftInYaw = true;
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

  template <typename Number>
  void RunFrame(std::vector<Number> &lastRead) {
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
        m_compGrip, lastRead[13] > 0.4, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compSystem, lastRead[14] > 0.4, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compAppMenu, lastRead[15] > 0.4, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTrackpadClick, lastRead[16] > 0.4,
        0);

    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrigger, float(lastRead[17]), 0);
    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrackpadX, float(lastRead[18]), 0);
    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrackpadY, float(lastRead[19]), 0);

    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTrackpadTouch, lastRead[20] > 0.4,
        0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTriggerClick, lastRead[21] > 0.4,
        0);

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
class TrackerDriver : public hobovr::HobovrDevice<true> {
public:
  TrackerDriver(std::string myserial, const std::shared_ptr<SockReceiver::DriverReceiver> remotePoser):
  HobovrDevice(myserial, "hobovr_tracker_m", remotePoser) {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_tracker_mt0";
    m_sBindPath = "{hobovr}/input/hobovr_tracker_profile.json";

    m_Pose.poseTimeOffset = 0;
    m_Pose.poseIsValid = true;
    m_Pose.deviceIsConnected = true;
    m_Pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
    m_Pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
    m_Pose.vecPosition[0] = 0.;

    m_Pose.vecPosition[2] = 0.;
    m_Pose.willDriftInYaw = true;
  }

  virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
    HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

    vr::VRProperties()->SetInt32Property(m_ulPropertyContainer,
                                         Prop_ControllerRoleHint_Int32,
                                         TrackedControllerRole_RightHand);

    return VRInitError_None;
  }

  template <typename Number>
  void RunFrame(std::vector<Number> &lastRead) {
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

struct HoboDevice_t
{
  enum{HMD, CNTRLR, TRKR} tag;
  bool active;
  union {
    HeadsetDriver* hmd;
    ControllerDriver* controller;
    TrackerDriver* tracker;
  };
};

class CServerDriver_hobovr : public IServerTrackedDeviceProvider {
public:
  CServerDriver_hobovr() {
    m_bMyThreadKeepAlive = false;
    m_pMyTread = nullptr;
  }
  virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
  virtual void Cleanup();
  virtual const char *const *GetInterfaceVersions() {
    return vr::k_InterfaceVersions;
  }
  virtual bool ShouldBlockStandbyMode() { return false; }
  virtual void EnterStandby() {}
  virtual void LeaveStandby() {}
  virtual void RunFrame() {}
  static void myThreadEnter(CServerDriver_hobovr *pClass) {
    pClass->myTrackingThread();
  }
  void myTrackingThread();

private:
  std::vector<HoboDevice_t> m_vDevices;

  std::shared_ptr<SockReceiver::DriverReceiver> remotePoser;

  bool m_bMyThreadKeepAlive;
  std::thread *m_pMyTread;
};

EVRInitError CServerDriver_hobovr::Init(vr::IVRDriverContext *pDriverContext) {
  VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
  InitDriverLog(vr::VRDriverLog());

  std::string uduThing;
  char buf[1024];
  vr::VRSettings()->GetString(k_pch_Hobovr_Section,
                              k_pch_Hobovr_UduDeviceManifestList_String, buf,
                              sizeof(buf));
  uduThing = buf;
  DriverLog("device manifest list: '%s'\n", uduThing.c_str());

  try{
    remotePoser = std::make_shared<SockReceiver::DriverReceiver>(uduThing);
    remotePoser->start();
  } catch (...){
    DriverLog("remotePoser broke on create or broke on start, either way you're fucked\n");
    return VRInitError_Init_WebServerFailed;
  }

  int counter_hmd = 0;
  int counter_cntrlr = 0;
  int counter_trkr = 0;
  int controller_hs = 1;

  for (std::string i:remotePoser->device_list) {
    if (i == "h") {
      HoboDevice_t temp = {HoboDevice_t::HMD, 1};
      temp.hmd = new HeadsetDriver("h" + std::to_string(counter_hmd));

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back().hmd->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD,
                    m_vDevices.back().hmd);

      counter_hmd++;

    } else if (i == "c") {
      HoboDevice_t temp = {HoboDevice_t::CNTRLR, 1};
      temp.controller = new ControllerDriver(controller_hs, "c" + std::to_string(counter_cntrlr), remotePoser);

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back().controller->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller,
                    m_vDevices.back().controller);

      controller_hs = (controller_hs) ? 0 : 1;
      counter_cntrlr++;

    } else if (i == "t") {
      HoboDevice_t temp = {HoboDevice_t::TRKR, 1};
      temp.tracker = new TrackerDriver("t" + std::to_string(counter_trkr), remotePoser);

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back().tracker->GetSerialNumber().c_str(), vr::TrackedDeviceClass_GenericTracker,
                    m_vDevices.back().tracker);

      counter_trkr++;

    } else {
      DriverLog("unsopported device type: %s", i);
      return VRInitError_VendorSpecific_HmdFound_ConfigFailedSanityCheck;
    }
  }

  m_bMyThreadKeepAlive = true;
  std::this_thread::sleep_for(std::chrono::microseconds(20000));
  m_pMyTread = new std::thread(myThreadEnter, this);

  if (!m_pMyTread) {
    DriverLog("failed to start tracking thread\n");
    return VRInitError_Driver_Failed;
  }

  return VRInitError_None;
}

void CServerDriver_hobovr::Cleanup() {

  m_bMyThreadKeepAlive = false;
  if (m_pMyTread) {
    m_pMyTread->join();
    delete m_pMyTread;
    m_pMyTread = nullptr;
  }

  for (auto &i: m_vDevices) {
    i.active = 0;
    switch (i.tag) {
      case HoboDevice_t::HMD:
        delete i.hmd;
        i.hmd = nullptr;
        break;

      case HoboDevice_t::CNTRLR:
        delete i.controller;
        i.controller = nullptr;
        break;

      case HoboDevice_t::TRKR:
        delete i.tracker;
        i.tracker = nullptr;
        break;
    }
  }

  CleanupDriverLog();
  VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void CServerDriver_hobovr::myTrackingThread() {
#if defined(_WINDOWS)

  DWORD dwError, dwThreadPri;

  if(!SetThreadPriority(GetCurrentThread(), 15))
  {
    dwError = GetLastError();
    DriverLog("failed to set tracking thread priority to 15: %d", dwError);
  }

  dwThreadPri = GetThreadPriority(GetCurrentThread());
  DriverLog("current tracking thread priority: %d", dwThreadPri);
#endif


  std::vector<std::vector<double>> tempPose;
  vr::VREvent_t vrEvent;
  auto iTotal_devices = m_vDevices.size();

  while (m_bMyThreadKeepAlive) {
    tempPose = remotePoser->get_pose();

    for (int i=0; i<iTotal_devices; i++){
      if (m_vDevices[i].active) {
        switch (m_vDevices[i].tag)
        {
          case HoboDevice_t::HMD:
            m_vDevices[i].hmd->RunFrame(tempPose[i]);
            break;

          case HoboDevice_t::CNTRLR:
            m_vDevices[i].controller->RunFrame(tempPose[i]);
            break;

          case HoboDevice_t::TRKR:
            m_vDevices[i].tracker->RunFrame(tempPose[i]);
            break;

        }
      }
    }

    while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
      for (auto &i : m_vDevices){
        if (i.active) {
          switch (i.tag){
            case HoboDevice_t::HMD:
              i.hmd->ProcessEvent(vrEvent);
              break;
            case HoboDevice_t::CNTRLR:
              i.controller->ProcessEvent(vrEvent);
              break;
            case HoboDevice_t::TRKR:
              i.tracker->ProcessEvent(vrEvent);
              break;
          }
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::microseconds(200));
  }
}

CServerDriver_hobovr g_serverDriverNull;

//-----------------------------------------------------------------------------
// Purpose: driverFactory
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void *HmdDriverFactory(const char *pInterfaceName,
                                      int *pReturnCode) {
  if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName)) {
    return &g_serverDriverNull;
  }
  // if (0 == strcmp(IVRWatchdogProvider_Version, pInterfaceName)) {
  //   return &g_watchdogDriverNull;
  // }

  if (pReturnCode)
    *pReturnCode = VRInitError_Init_InterfaceNotFound;

  return NULL;
}
