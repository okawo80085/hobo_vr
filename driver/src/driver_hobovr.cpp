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

  virtual ~HeadsetDriver() {}

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
// Purpose:controllerDriver
//-----------------------------------------------------------------------------
class ControllerDriver : public vr::ITrackedDeviceServerDriver {
public:
  ControllerDriver(bool side, std::string myserial, SockReceiver::DriverReceiver *remotePoser) {
    this->remotePoser = remotePoser;
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

    m_sSerialNumber = myserial;
    m_sModelNumber = "hobovr_controller_m" + m_sSerialNumber;
    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_controller_mc0";

    poseController.poseTimeOffset = 0;
    poseController.poseIsValid = true;
    poseController.deviceIsConnected = true;
    poseController.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
    poseController.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
    poseController.vecPosition[0] = 0.;
    if (side) {
      poseController.vecPosition[1] = 1.;
    } else {
      poseController.vecPosition[1] = 1.1;
    }

    poseController.vecPosition[2] = 0.;
    poseController.willDriftInYaw = true;
    handSide_ = side;
  }

  virtual ~ControllerDriver() {}

  virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
    m_unObjectId = unObjectId;
    m_ulPropertyContainer =
        vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
    vr::VRProperties()->SetStringProperty(m_ulPropertyContainer,
                                          Prop_RenderModelName_String,
                                          m_sRenderModelPath.c_str());

    // return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
    vr::VRProperties()->SetUint64Property(m_ulPropertyContainer,
                                          Prop_CurrentUniverseId_Uint64, 2);

    // even though we won't ever track we want to pretend to be the right hand
    // so binding will work as expected
    if (handSide_) {
      vr::VRProperties()->SetInt32Property(m_ulPropertyContainer,
                                           Prop_ControllerRoleHint_Int32,
                                           TrackedControllerRole_RightHand);
    } else {
      vr::VRProperties()->SetInt32Property(m_ulPropertyContainer,
                                           Prop_ControllerRoleHint_Int32,
                                           TrackedControllerRole_LeftHand);
    }

    // this file tells the UI what to show the user for binding this controller
    // as well as what default bindings should
    // be for legacy or other apps
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer, Prop_InputProfilePath_String,
        "{hobovr}/input/hobovr_controller_profile.json");

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

    // create our haptic component
    vr::VRDriverInput()->CreateHapticComponent(m_ulPropertyContainer,
                                               "/output/haptic", &m_compHaptic);

    return VRInitError_None;
  }

  virtual void Deactivate() {
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
  }

  virtual void EnterStandby() {}

  void *GetComponent(const char *pchComponentNameAndVersion) {
    // override this to add a component to a driver
    return NULL;
  }

  virtual void PowerOff() {}

  /** debug request from a client */
  virtual void DebugRequest(const char *pchRequest, char *pchResponseBuffer,
                            uint32_t unResponseBufferSize) {
    if (unResponseBufferSize >= 1)
      pchResponseBuffer[0] = 0;
  }

  virtual DriverPose_t GetPose() { return poseController; } // this is called like once 

  void RunFrame(std::vector<double> &lastRead) {
    // update all the things

    poseController.result = TrackingResult_Running_OK;

    poseController.vecPosition[0] = lastRead[0];
    poseController.vecPosition[1] = lastRead[1];
    poseController.vecPosition[2] = lastRead[2];

    poseController.qRotation =
        HmdQuaternion_Init(lastRead[3],
                           lastRead[4],
                           lastRead[5],
                           lastRead[6]);

    poseController.vecVelocity[0] = lastRead[7];
    poseController.vecVelocity[1] = lastRead[8];
    poseController.vecVelocity[2] = lastRead[9];

    poseController.vecAngularVelocity[0] =
        lastRead[10];
    poseController.vecAngularVelocity[1] =
        lastRead[11];
    poseController.vecAngularVelocity[2] =
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
          m_unObjectId, poseController, sizeof(DriverPose_t));
    }
  }

  void ProcessEvent(const vr::VREvent_t &vrEvent) {
    switch (vrEvent.eventType) {
    case vr::VREvent_Input_HapticVibration: {
      if (vrEvent.data.hapticVibration.componentHandle == m_compHaptic) {
        // haptic!
        if (remotePoser != NULL) {
          remotePoser->send2((m_sSerialNumber + "\n").c_str());
        }
      }
    } break;
    }
  }

  std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
  vr::TrackedDeviceIndex_t m_unObjectId;
  vr::PropertyContainerHandle_t m_ulPropertyContainer;

  vr::VRInputComponentHandle_t m_compGrip;
  vr::VRInputComponentHandle_t m_compSystem;
  vr::VRInputComponentHandle_t m_compAppMenu;
  vr::VRInputComponentHandle_t m_compTrigger;
  vr::VRInputComponentHandle_t m_compTrackpadX;
  vr::VRInputComponentHandle_t m_compTrackpadY;
  vr::VRInputComponentHandle_t m_compTrackpadTouch;
  vr::VRInputComponentHandle_t m_compTriggerClick;
  vr::VRInputComponentHandle_t m_compTrackpadClick;
  vr::VRInputComponentHandle_t m_compHaptic;

  std::string m_sSerialNumber;
  std::string m_sModelNumber;
  std::string m_sRenderModelPath;

  DriverPose_t poseController;
  bool handSide_;
  SockReceiver::DriverReceiver *remotePoser;
};


//-----------------------------------------------------------------------------
// Purpose: trackerDriver
//-----------------------------------------------------------------------------
class TrackerDriver : public vr::ITrackedDeviceServerDriver {
public:
  TrackerDriver(std::string myserial, SockReceiver::DriverReceiver *remotePoser) {
    this->remotePoser = remotePoser;
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

    m_sSerialNumber = myserial;
    m_sModelNumber = "hobovr_tracker_m" + m_sSerialNumber;
    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_tracker_mt0";

    poseTracker.poseTimeOffset = 0;
    poseTracker.poseIsValid = true;
    poseTracker.deviceIsConnected = true;
    poseTracker.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
    poseTracker.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
    poseTracker.vecPosition[0] = 0.;

    poseTracker.vecPosition[2] = 0.;
    poseTracker.willDriftInYaw = true;
  }

  virtual ~TrackerDriver() {}

  virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
    m_unObjectId = unObjectId;
    m_ulPropertyContainer =
        vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
    vr::VRProperties()->SetStringProperty(m_ulPropertyContainer,
                                          Prop_RenderModelName_String,
                                          m_sRenderModelPath.c_str());

    // return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
    vr::VRProperties()->SetUint64Property(m_ulPropertyContainer,
                                          Prop_CurrentUniverseId_Uint64, 2);

    // even though we won't ever track we want to pretend to be the right hand
    // so binding will work as expected
    vr::VRProperties()->SetInt32Property(m_ulPropertyContainer,
                                         Prop_ControllerRoleHint_Int32,
                                         TrackedControllerRole_RightHand);


    // this file tells the UI what to show the user for binding this controller
    // as well as what default bindings should
    // be for legacy or other apps
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer, Prop_InputProfilePath_String,
        "{hobovr}/input/hobovr_tracker_profile.json");

    return VRInitError_None;
  }

  virtual void Deactivate() {
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
  }

  virtual void EnterStandby() {}

  void *GetComponent(const char *pchComponentNameAndVersion) {
    // override this to add a component to a driver
    return NULL;
  }

  virtual void PowerOff() {}

  /** debug request from a client */
  virtual void DebugRequest(const char *pchRequest, char *pchResponseBuffer,
                            uint32_t unResponseBufferSize) {
    if (unResponseBufferSize >= 1)
      pchResponseBuffer[0] = 0;
  }

  virtual DriverPose_t GetPose() { return poseTracker; } // this is called like once 

  void RunFrame(std::vector<double> &lastRead) {
    // update all the things

    poseTracker.result = TrackingResult_Running_OK;

    poseTracker.vecPosition[0] = lastRead[0];
    poseTracker.vecPosition[1] = lastRead[1];
    poseTracker.vecPosition[2] = lastRead[2];

    poseTracker.qRotation =
        HmdQuaternion_Init(lastRead[3],
                           lastRead[4],
                           lastRead[5],
                           lastRead[6]);

    poseTracker.vecVelocity[0] = lastRead[7];
    poseTracker.vecVelocity[1] = lastRead[8];
    poseTracker.vecVelocity[2] = lastRead[9];

    poseTracker.vecAngularVelocity[0] =
        lastRead[10];
    poseTracker.vecAngularVelocity[1] =
        lastRead[11];
    poseTracker.vecAngularVelocity[2] =
        lastRead[12];

    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
      vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
          m_unObjectId, poseTracker, sizeof(DriverPose_t));
    }
  }

  std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
  vr::TrackedDeviceIndex_t m_unObjectId;
  vr::PropertyContainerHandle_t m_ulPropertyContainer;

  std::string m_sSerialNumber;
  std::string m_sModelNumber;
  std::string m_sRenderModelPath;

  DriverPose_t poseTracker;
  SockReceiver::DriverReceiver *remotePoser;
};

//-----------------------------------------------------------------------------
// Purpose: serverDriver
//-----------------------------------------------------------------------------

struct HoboDevice
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
  std::vector<HoboDevice> m_vDevices;

  SockReceiver::DriverReceiver *remotePoser;

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
    remotePoser = new SockReceiver::DriverReceiver(uduThing);
    remotePoser->start();
  } catch (...){
    remotePoser = NULL;
    DriverLog("remotePoser broke on create or broke on start, either way you're fucked\n");
  }

  int counter_hmd = 0;
  int counter_cntrlr = 0;
  int counter_trkr = 0;
  int controller_hs = 1;

  for (std::string i:remotePoser->device_list) {
    if (i == "h") {
      HoboDevice temp = {HoboDevice::HMD, 1};
      temp.hmd = new HeadsetDriver("h" + std::to_string(counter_hmd));

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back().hmd->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD,
                    m_vDevices.back().hmd);

      counter_hmd++;

    } else if (i == "c") {
      HoboDevice temp = {HoboDevice::CNTRLR, 1};
      temp.controller = new ControllerDriver(controller_hs, "c" + std::to_string(counter_cntrlr), remotePoser);

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back().controller->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller,
                    m_vDevices.back().controller);

      controller_hs = (controller_hs) ? 0 : 1;
      counter_cntrlr++;

    } else if (i == "t") {
      HoboDevice temp = {HoboDevice::TRKR, 1};
      temp.tracker = new TrackerDriver("t" + std::to_string(counter_trkr), remotePoser);

      m_vDevices.push_back(temp);
      vr::VRServerDriverHost()->TrackedDeviceAdded(
                    m_vDevices.back().tracker->GetSerialNumber().c_str(), vr::TrackedDeviceClass_GenericTracker,
                    m_vDevices.back().tracker);

      counter_trkr++;

    } else {
      DriverLog("unsopported device type: %s", i);
      return VRInitError_Driver_Failed;
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
  CleanupDriverLog();

  m_bMyThreadKeepAlive = false;
  if (m_pMyTread) {
    m_pMyTread->join();
    delete m_pMyTread;
    m_pMyTread = nullptr;
  }

  delete remotePoser;
  remotePoser = NULL;

  for (auto &i: m_vDevices) {
    switch (i.tag) {
      case HoboDevice::HMD:
        delete i.hmd;
        i.hmd = NULL;
        i.active = 0;
        break;

      case HoboDevice::CNTRLR:
        delete i.controller;
        i.controller = NULL;
        i.active = 0;
        break;

      case HoboDevice::TRKR:
        delete i.tracker;
        i.tracker = NULL;
        i.active = 0;
        break;
    }
  }

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

  while (m_bMyThreadKeepAlive) {
    tempPose = remotePoser->get_pose();

    for (int i=0; i<m_vDevices.size(); i++){
      if (m_vDevices[i].active) {
        switch (m_vDevices[i].tag)
        {
          case HoboDevice::HMD:
            m_vDevices[i].hmd->RunFrame(tempPose[i]);
            break;

          case HoboDevice::CNTRLR:
            m_vDevices[i].controller->RunFrame(tempPose[i]);
            break;

          case HoboDevice::TRKR:
            m_vDevices[i].tracker->RunFrame(tempPose[i]);
            break;

        }
      }
    }

    while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
      for (auto &i : m_vDevices){
        if (i.active && i.tag == HoboDevice::CNTRLR) {
          i.controller->ProcessEvent(vrEvent);
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
