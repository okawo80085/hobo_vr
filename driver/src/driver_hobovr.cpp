//============ Copyright (c) okawo, All rights reserved.
//============

//#include "openvr.h"
#include "openvr_driver.h"
//#include "openvr_capi.h"
#include "driverlog.h"

#include <chrono>
#include <thread>
#include <vector>

#include "ref/receiver.h"

#if defined(_WINDOWS)
#include <windows.h>

#else

#define _stricmp strcasecmp

#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

// #include "ref/controllerDriverRef.h"

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

inline void HmdMatrix_SetIdentity(HmdMatrix34_t *pMatrix) {
  pMatrix->m[0][0] = 1.f;
  pMatrix->m[0][1] = 0.f;
  pMatrix->m[0][2] = 0.f;
  pMatrix->m[0][3] = 0.f;
  pMatrix->m[1][0] = 0.f;
  pMatrix->m[1][1] = 1.f;
  pMatrix->m[1][2] = 0.f;
  pMatrix->m[1][3] = 0.f;
  pMatrix->m[2][0] = 0.f;
  pMatrix->m[2][1] = 0.f;
  pMatrix->m[2][2] = 1.f;
  pMatrix->m[2][3] = 0.f;
}

// keys for use with the settings API
static const char *const k_pch_Hobovr_Section = "driver_hobovr";
static const char *const k_pch_Hobovr_SerialNumber_String = "serialNumber";
static const char *const k_pch_Hobovr_ModelNumber_String = "modelNumber";
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

//-----------------------------------------------------------------------------
// Purpose: watchdogDriver
//-----------------------------------------------------------------------------

class CWatchdogDriver_Sample : public IVRWatchdogProvider {
public:
  CWatchdogDriver_Sample() { m_pWatchdogThread = nullptr; }

  virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
  virtual void Cleanup();

private:
  std::thread *m_pWatchdogThread;
};

CWatchdogDriver_Sample g_watchdogDriverNull;

// bool g_bExiting = false;

// void WatchdogThreadFunction() {
//   while (!g_bExiting) {
// #if defined(_WINDOWS)
//     // on windows send the event when the Y key is pressed.
//     if ((0x01 & GetAsyncKeyState(VK_MULTIPLY)) != 0) {
//       // Y key was pressed.
//       vr::VRWatchdogHost()->WatchdogWakeUp(vr::TrackedDeviceClass_HMD);
//     }
//     std::this_thread::sleep_for(std::chrono::microseconds(500));
// #else
//     // for the other platforms, just send one every five seconds
//     std::this_thread::sleep_for(std::chrono::seconds(5));
//     vr::VRWatchdogHost()->WatchdogWakeUp(vr::TrackedDeviceClass_HMD);
// #endif
//   }
// }

EVRInitError
CWatchdogDriver_Sample::Init(vr::IVRDriverContext *pDriverContext) {
  VR_INIT_WATCHDOG_DRIVER_CONTEXT(pDriverContext);
  InitDriverLog(vr::VRDriverLog());

  // Watchdog mode on Windows starts a thread that listens for the '*' key on
  // the keyboard to
  // be pressed. A real driver should wait for a system button event or
  // something else from the
  // the hardware that signals that the VR system should start up.
  // g_bExiting = false;
  // m_pWatchdogThread = new std::thread(WatchdogThreadFunction);
  // if (!m_pWatchdogThread) {
  //   DriverLog("Unable to create watchdog thread\n");
  //   return VRInitError_Driver_Failed;
  // }

  return VRInitError_None;
}

void CWatchdogDriver_Sample::Cleanup() {
  // g_bExiting = true;
  // if (m_pWatchdogThread) {
  //   m_pWatchdogThread->join();
  //   delete m_pWatchdogThread;
  //   m_pWatchdogThread = nullptr;
  // }

  CleanupDriverLog();
}

//-----------------------------------------------------------------------------
// Purpose: hmdDriver
//-----------------------------------------------------------------------------

class HeadsetDriver : public vr::ITrackedDeviceServerDriver,
                      public vr::IVRDisplayComponent {
public:
  HeadsetDriver(SockReceiver::DriverReceiver *remotePoser) {
    this->remotePoser = remotePoser;
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

    DriverLog("Using settings values\n");
    m_flIPD = vr::VRSettings()->GetFloat(k_pch_SteamVR_Section,
                                         k_pch_SteamVR_IPD_Float);

    char buf[1024];
    vr::VRSettings()->GetString(k_pch_Hobovr_Section,
                                k_pch_Hobovr_SerialNumber_String, buf,
                                sizeof(buf));
    m_sSerialNumber = buf;

    vr::VRSettings()->GetString(k_pch_Hobovr_Section,
                                k_pch_Hobovr_ModelNumber_String, buf,
                                sizeof(buf));
    m_sModelNumber = buf;

    m_nWindowX = vr::VRSettings()->GetInt32(k_pch_Hobovr_Section,
                                            k_pch_Hobovr_WindowX_Int32);
    m_nWindowY = vr::VRSettings()->GetInt32(k_pch_Hobovr_Section,
                                            k_pch_Hobovr_WindowY_Int32);
    m_nWindowWidth = vr::VRSettings()->GetInt32(k_pch_Hobovr_Section,
                                                k_pch_Hobovr_WindowWidth_Int32);
    m_nWindowHeight = vr::VRSettings()->GetInt32(
        k_pch_Hobovr_Section, k_pch_Hobovr_WindowHeight_Int32);
    m_nRenderWidth = vr::VRSettings()->GetInt32(k_pch_Hobovr_Section,
                                                k_pch_Hobovr_RenderWidth_Int32);
    m_nRenderHeight = vr::VRSettings()->GetInt32(
        k_pch_Hobovr_Section, k_pch_Hobovr_RenderHeight_Int32);
    m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(
        k_pch_Hobovr_Section, k_pch_Hobovr_SecondsFromVsyncToPhotons_Float);
    m_flDisplayFrequency = vr::VRSettings()->GetFloat(
        k_pch_Hobovr_Section, k_pch_Hobovr_DisplayFrequency_Float);

    m_fDistortionK1 = vr::VRSettings()->GetFloat(
        k_pch_Hobovr_Section, k_pch_Hobovr_DistortionK1_Float);
    m_fDistortionK2 = vr::VRSettings()->GetFloat(
        k_pch_Hobovr_Section, k_pch_Hobovr_DistortionK2_Float);
    m_fZoomWidth = vr::VRSettings()->GetFloat(k_pch_Hobovr_Section,
                                              k_pch_Hobovr_ZoomWidth_Float);
    m_fZoomHeight = vr::VRSettings()->GetFloat(k_pch_Hobovr_Section,
                                               k_pch_Hobovr_ZoomHeight_Float);

    DriverLog("Serial Number: %s\n", m_sSerialNumber.c_str());
    DriverLog("Model Number: %s\n", m_sModelNumber.c_str());
    DriverLog("Window: %d %d %d %d\n", m_nWindowX, m_nWindowY, m_nWindowWidth,
              m_nWindowHeight);
    DriverLog("Render Target: %d %d\n", m_nRenderWidth, m_nRenderHeight);
    DriverLog("Seconds from Vsync to Photons: %f\n",
              m_flSecondsFromVsyncToPhotons);
    DriverLog("Display Frequency: %f\n", m_flDisplayFrequency);
    DriverLog("IPD: %f\n", m_flIPD);

    pose.poseTimeOffset = 0;
    pose.poseIsValid = true;
    pose.deviceIsConnected = true;
    pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
    pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
    pose.vecPosition[0] = 0.;
    pose.vecPosition[1] = 0.;
    pose.vecPosition[2] = 0.;
    pose.willDriftInYaw = true;
  }

  virtual ~HeadsetDriver() {}

  virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
    m_unObjectId = unObjectId;
    m_ulPropertyContainer =
        vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
    vr::VRProperties()->SetStringProperty(m_ulPropertyContainer,
                                          Prop_RenderModelName_String,
                                          m_sModelNumber.c_str());
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

    // return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
    vr::VRProperties()->SetUint64Property(m_ulPropertyContainer,
                                          Prop_CurrentUniverseId_Uint64, 2);

    // avoid "not fullscreen" warnings from vrmonitor
    vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
                                        Prop_IsOnDesktop_Bool, false);

    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer, Prop_InputProfilePath_String,
        "{hobovr}/input/hobovr_hmd_profile.json");

    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer, "/input/system/click", &m_compSystem);

    return VRInitError_None;
  }

  virtual void Deactivate() {
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
  }

  virtual void EnterStandby() {}

  void *GetComponent(const char *pchComponentNameAndVersion) {
    if (!_stricmp(pchComponentNameAndVersion,
                  vr::IVRDisplayComponent_Version)) {
      return (vr::IVRDisplayComponent *)this;
    }
    else if (!_stricmp(pchComponentNameAndVersion,
                  vr::ITrackedDeviceServerDriver_Version)) {
      return (vr::ITrackedDeviceServerDriver *)this;
    }

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

  virtual void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth,
                               uint32_t *pnHeight) {
    *pnX = m_nWindowX;
    *pnY = m_nWindowY;
    *pnWidth = m_nWindowWidth;
    *pnHeight = m_nWindowHeight;
  }

  virtual bool IsDisplayOnDesktop() { return true; }

  virtual bool IsDisplayRealDisplay() { return false; }

  virtual void GetRecommendedRenderTargetSize(uint32_t *pnWidth,
                                              uint32_t *pnHeight) {
    *pnWidth = m_nRenderWidth;
    *pnHeight = m_nRenderHeight;
  }

  virtual void GetEyeOutputViewport(EVREye eEye, uint32_t *pnX, uint32_t *pnY,
                                    uint32_t *pnWidth, uint32_t *pnHeight) {
    *pnY = 0;
    *pnWidth = m_nWindowWidth / 2;
    *pnHeight = m_nWindowHeight;

    if (eEye == Eye_Left) {
      *pnX = 0;
    } else {
      *pnX = m_nWindowWidth / 2;
    }
  }

  virtual void GetProjectionRaw(EVREye eEye, float *pfLeft, float *pfRight,
                                float *pfTop, float *pfBottom) {
    *pfLeft = -1.0;
    *pfRight = 1.0;
    *pfTop = -1.0;
    *pfBottom = 1.0;
  }

  virtual DistortionCoordinates_t ComputeDistortion(EVREye eEye, float fU,
                                                    float fV) {
    DistortionCoordinates_t coordinates;

    // Distortion for lens implementation from
    // https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc
    float hX;
    float hY;
    double rr;
    double r2;
    double theta;

    rr = sqrt((fU - 0.5f) * (fU - 0.5f) + (fV - 0.5f) * (fV - 0.5f));
    r2 = rr * (1 + m_fDistortionK1 * (rr * rr) +
               m_fDistortionK2 * (rr * rr * rr * rr));
    theta = atan2(fU - 0.5f, fV - 0.5f);
    hX = float(sin(theta) * r2) * m_fZoomWidth;
    hY = float(cos(theta) * r2) * m_fZoomHeight;

    coordinates.rfBlue[0] = hX + 0.5f;
    coordinates.rfBlue[1] = hY + 0.5f;
    coordinates.rfGreen[0] = hX + 0.5f;
    coordinates.rfGreen[1] = hY + 0.5f;
    coordinates.rfRed[0] = hX + 0.5f;
    coordinates.rfRed[1] = hY + 0.5f;

    return coordinates;
  }

  virtual DriverPose_t GetPose() { return pose; }

  void RunFrame(std::vector<double> &lastRead) {
    pose.result = TrackingResult_Running_OK;
    pose.vecPosition[0] = lastRead[0];
    pose.vecPosition[1] = lastRead[1];
    pose.vecPosition[2] = lastRead[2];

    pose.qRotation =
        HmdQuaternion_Init(lastRead[3], lastRead[4],
                           lastRead[5], lastRead[6]);

    pose.vecVelocity[0] = lastRead[7];
    pose.vecVelocity[1] = lastRead[8];
    pose.vecVelocity[2] = lastRead[9];

    pose.vecAngularVelocity[0] = lastRead[10];
    pose.vecAngularVelocity[1] = lastRead[11];
    pose.vecAngularVelocity[2] = lastRead[12];

    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
      vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
          m_unObjectId, pose, sizeof(DriverPose_t));
    }
  }

  std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
  vr::TrackedDeviceIndex_t m_unObjectId;
  vr::PropertyContainerHandle_t m_ulPropertyContainer;

  vr::VRInputComponentHandle_t m_compSystem;


  std::string m_sSerialNumber;
  std::string m_sModelNumber;

  int32_t m_nWindowX;
  int32_t m_nWindowY;
  int32_t m_nWindowWidth;
  int32_t m_nWindowHeight;
  int32_t m_nRenderWidth;
  int32_t m_nRenderHeight;
  float m_flSecondsFromVsyncToPhotons;
  float m_flDisplayFrequency;
  float m_flIPD;

  float m_fDistortionK1;
  float m_fDistortionK2;
  float m_fZoomWidth;
  float m_fZoomHeight;

  SockReceiver::DriverReceiver *remotePoser;

  DriverPose_t pose;
};

//-----------------------------------------------------------------------------
// Purpose:controllerDriver
//-----------------------------------------------------------------------------
class ControllerDriver : public vr::ITrackedDeviceServerDriver {
public:
  ControllerDriver(bool side, SockReceiver::DriverReceiver *remotePoser) {
    this->remotePoser = remotePoser;
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

    if (side) {
      m_sSerialNumber = "nut666";
    } else {
      m_sSerialNumber = "nut999";
    }
    m_sModelNumber = "hobovr_controller_m1";

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
                                          m_sModelNumber.c_str());

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
    if (!_stricmp(pchComponentNameAndVersion,
                  vr::ITrackedDeviceServerDriver_Version)) {
      return (vr::ITrackedDeviceServerDriver *)this;
    }

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

    int i_indexOffset = 13;
    if (!handSide_) {
      i_indexOffset += 22;
    }

    poseController.result = TrackingResult_Running_OK;

    poseController.vecPosition[0] = lastRead[(i_indexOffset + 0)];
    poseController.vecPosition[1] = lastRead[(i_indexOffset + 1)];
    poseController.vecPosition[2] = lastRead[(i_indexOffset + 2)];

    poseController.qRotation =
        HmdQuaternion_Init(lastRead[(i_indexOffset + 3)],
                           lastRead[(i_indexOffset + 4)],
                           lastRead[(i_indexOffset + 5)],
                           lastRead[(i_indexOffset + 6)]);

    poseController.vecVelocity[0] = lastRead[(i_indexOffset + 7)];
    poseController.vecVelocity[1] = lastRead[(i_indexOffset + 8)];
    poseController.vecVelocity[2] = lastRead[(i_indexOffset + 9)];

    poseController.vecAngularVelocity[0] =
        lastRead[(i_indexOffset + 10)];
    poseController.vecAngularVelocity[1] =
        lastRead[(i_indexOffset + 11)];
    poseController.vecAngularVelocity[2] =
        lastRead[(i_indexOffset + 12)];

    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compGrip, lastRead[(i_indexOffset + 13)] > 0.4, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compSystem, lastRead[(i_indexOffset + 14)] > 0.4, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compAppMenu, lastRead[(i_indexOffset + 15)] > 0.4, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTrackpadClick, lastRead[(i_indexOffset + 16)] > 0.4,
        0);

    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrigger, float(lastRead[(i_indexOffset + 17)]), 0);
    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrackpadX, float(lastRead[(i_indexOffset + 18)]), 0);
    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrackpadY, float(lastRead[(i_indexOffset + 19)]), 0);

    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTrackpadTouch, lastRead[(i_indexOffset + 20)] > 0.4,
        0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTriggerClick, lastRead[(i_indexOffset + 21)] > 0.4,
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
          if (handSide_) {
            remotePoser->send2("driver:buzz right\n");
          } else {
            remotePoser->send2("driver:buzz left\n");
          }
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

  DriverPose_t poseController;
  bool handSide_;
  SockReceiver::DriverReceiver *remotePoser;
};

//-----------------------------------------------------------------------------
// Purpose: serverDriver
//-----------------------------------------------------------------------------
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
  HeadsetDriver *m_pNullHmdLatest = nullptr;
  ControllerDriver *m_pRightController = nullptr;
  ControllerDriver *m_pLeftController = nullptr;

  SockReceiver::DriverReceiver *remotePoser;

  bool m_bMyThreadKeepAlive;
  std::thread *m_pMyTread;
};

EVRInitError CServerDriver_hobovr::Init(vr::IVRDriverContext *pDriverContext) {
  remotePoser = new SockReceiver::DriverReceiver(57);
  VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
  remotePoser->start();

  InitDriverLog(vr::VRDriverLog());

  m_pNullHmdLatest = new HeadsetDriver(remotePoser);
  vr::VRServerDriverHost()->TrackedDeviceAdded(
      m_pNullHmdLatest->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD,
      m_pNullHmdLatest);

  m_pRightController = new ControllerDriver(1, remotePoser);
  m_pLeftController = new ControllerDriver(0, remotePoser);

  vr::VRServerDriverHost()->TrackedDeviceAdded(
      m_pRightController->GetSerialNumber().c_str(),
      vr::TrackedDeviceClass_Controller, m_pRightController);
  vr::VRServerDriverHost()->TrackedDeviceAdded(
      m_pLeftController->GetSerialNumber().c_str(),
      vr::TrackedDeviceClass_Controller, m_pLeftController);

  m_bMyThreadKeepAlive = true;
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

  delete m_pNullHmdLatest;
  delete m_pRightController;
  delete m_pLeftController;
  delete remotePoser;
  remotePoser = NULL;
  m_pNullHmdLatest = NULL;
  m_pRightController = NULL;
  m_pLeftController = NULL;
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


  std::vector<double> tempPose;
  while (m_bMyThreadKeepAlive) {

    tempPose = remotePoser->get_pose();
    if (!tempPose.empty())
    {
      if (m_pRightController != NULL) {
        m_pRightController->RunFrame(tempPose);
      }

      if (m_pLeftController != NULL) {
        m_pLeftController->RunFrame(tempPose);
      }

      if (m_pNullHmdLatest != NULL) {
        m_pNullHmdLatest->RunFrame(tempPose);
      }
    }

    vr::VREvent_t vrEvent;
    while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
      if (m_pRightController) {
        m_pRightController->ProcessEvent(vrEvent);
      }
      if (m_pLeftController) {
        m_pLeftController->ProcessEvent(vrEvent);
      }
    }


    std::this_thread::sleep_for(std::chrono::microseconds(1000));
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
  if (0 == strcmp(IVRWatchdogProvider_Version, pInterfaceName)) {
    return &g_watchdogDriverNull;
  }

  if (pReturnCode)
    *pReturnCode = VRInitError_Init_InterfaceNotFound;

  return NULL;
}
