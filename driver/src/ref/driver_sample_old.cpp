//============ Copyright (c) Valve Corporation, All rights reserved.
//============

//#include "openvr.h"
#include "openvr_driver.h"
//#include "openvr_capi.h"
#include "driverlog.h"

#include <chrono>
#include <thread>
#include <vector>

#define _WINSOCKAPI_ /* Prevent inclusion of winsock.h in windows.h */

#if defined(_WINDOWS)
#include <windows.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "ref/com_recv_win.h"
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
static const char *const k_pch_Sample_Section = "driver_sample";
static const char *const k_pch_Sample_SerialNumber_String = "serialNumber";
static const char *const k_pch_Sample_ModelNumber_String = "modelNumber";
static const char *const k_pch_Sample_WindowX_Int32 = "windowX";
static const char *const k_pch_Sample_WindowY_Int32 = "windowY";
static const char *const k_pch_Sample_WindowWidth_Int32 = "windowWidth";
static const char *const k_pch_Sample_WindowHeight_Int32 = "windowHeight";
static const char *const k_pch_Sample_RenderWidth_Int32 = "renderWidth";
static const char *const k_pch_Sample_RenderHeight_Int32 = "renderHeight";
static const char *const k_pch_Sample_SecondsFromVsyncToPhotons_Float =
    "secondsFromVsyncToPhotons";
static const char *const k_pch_Sample_DisplayFrequency_Float =
    "displayFrequency";

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

SP::socketPoser remotePoser(39);

bool g_bExiting = false;

void WatchdogThreadFunction() {
  while (!g_bExiting) {
#if defined(_WINDOWS)
    // on windows send the event when the Y key is pressed.
    if ((0x01 & GetAsyncKeyState(VK_MULTIPLY)) != 0) {
      // Y key was pressed.
      vr::VRWatchdogHost()->WatchdogWakeUp(vr::TrackedDeviceClass_HMD);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(500));
#else
    // for the other platforms, just send one every five seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));
    vr::VRWatchdogHost()->WatchdogWakeUp(vr::TrackedDeviceClass_HMD);
#endif
  }
}

EVRInitError
CWatchdogDriver_Sample::Init(vr::IVRDriverContext *pDriverContext) {
  VR_INIT_WATCHDOG_DRIVER_CONTEXT(pDriverContext);
  InitDriverLog(vr::VRDriverLog());

  // Watchdog mode on Windows starts a thread that listens for the 'Y' key on
  // the keyboard to be pressed. A real driver should wait for a system button
  // event or something else from the the hardware that signals that the VR
  // system should start up.
  g_bExiting = false;
  m_pWatchdogThread = new std::thread(WatchdogThreadFunction);
  if (!m_pWatchdogThread) {
    DriverLog("Unable to create watchdog thread\n");
    return VRInitError_Driver_Failed;
  }

  return VRInitError_None;
}

void CWatchdogDriver_Sample::Cleanup() {
  g_bExiting = true;
  if (m_pWatchdogThread) {
    m_pWatchdogThread->join();
    delete m_pWatchdogThread;
    m_pWatchdogThread = nullptr;
  }

  CleanupDriverLog();
}

//-----------------------------------------------------------------------------
// Purpose: hmdDriver
//-----------------------------------------------------------------------------

class CSampleDeviceDriver : public vr::ITrackedDeviceServerDriver,
                            public vr::IVRDisplayComponent {
public:
  CSampleDeviceDriver() {
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

    DriverLog("Using settings values\n");
    m_flIPD = vr::VRSettings()->GetFloat(k_pch_SteamVR_Section,
                                         k_pch_SteamVR_IPD_Float);

    char buf[1024];
    vr::VRSettings()->GetString(k_pch_Sample_Section,
                                k_pch_Sample_SerialNumber_String, buf,
                                sizeof(buf));
    m_sSerialNumber = buf;

    vr::VRSettings()->GetString(k_pch_Sample_Section,
                                k_pch_Sample_ModelNumber_String, buf,
                                sizeof(buf));
    m_sModelNumber = buf;

    m_nWindowX = vr::VRSettings()->GetInt32(k_pch_Sample_Section,
                                            k_pch_Sample_WindowX_Int32);
    m_nWindowY = vr::VRSettings()->GetInt32(k_pch_Sample_Section,
                                            k_pch_Sample_WindowY_Int32);
    m_nWindowWidth = vr::VRSettings()->GetInt32(k_pch_Sample_Section,
                                                k_pch_Sample_WindowWidth_Int32);
    m_nWindowHeight = vr::VRSettings()->GetInt32(
        k_pch_Sample_Section, k_pch_Sample_WindowHeight_Int32);
    m_nRenderWidth = vr::VRSettings()->GetInt32(k_pch_Sample_Section,
                                                k_pch_Sample_RenderWidth_Int32);
    m_nRenderHeight = vr::VRSettings()->GetInt32(
        k_pch_Sample_Section, k_pch_Sample_RenderHeight_Int32);
    m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(
        k_pch_Sample_Section, k_pch_Sample_SecondsFromVsyncToPhotons_Float);
    m_flDisplayFrequency = vr::VRSettings()->GetFloat(
        k_pch_Sample_Section, k_pch_Sample_DisplayFrequency_Float);

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
  }

  virtual ~CSampleDeviceDriver() {}

  virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
    srand(time(NULL));
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

    // Icons can be configured in code or automatically configured by an
    // external file "drivername\resources\driver.vrresources". Icon properties
    // NOT configured in code (post Activate) are then auto-configured by the
    // optional presence of a driver's
    // "drivername\resources\driver.vrresources". In this manner a driver can
    // configure their icons in a flexible data driven fashion by using an
    // external file.
    //
    // The structure of the driver.vrresources file allows a driver to
    // specialize their icons based on their HW. Keys matching the value in
    // "Prop_ModelNumber_String" are considered first, since the driver may have
    // model specific icons. An absence of a matching "Prop_ModelNumber_String"
    // then considers the ETrackedDeviceClass ("HMD", "Controller",
    // "GenericTracker", "TrackingReference") since the driver may have
    // specialized icons based on those device class names.
    //
    // An absence of either then falls back to the "system.vrresources" where
    // generic device class icons are then supplied.
    //
    // Please refer to "bin\drivers\sample\resources\driver.vrresources" which
    // contains this sample configuration.
    //
    // "Alias" is a reserved key and specifies chaining to another json block.
    //
    // In this sample configuration file (overly complex FOR EXAMPLE PURPOSES
    // ONLY)....
    //
    // "Model-v2.0" chains through the alias to "Model-v1.0" which chains
    // through the alias to "Model-v Defaults".
    //
    // Keys NOT found in "Model-v2.0" would then chase through the "Alias" to be
    // resolved in "Model-v1.0" and either resolve their or continue through the
    // alias. Thus "Prop_NamedIconPathDeviceAlertLow_String" in each model's
    // block represent a specialization specific for that "model". Keys in
    // "Model-v Defaults" are an example of mapping to the same states, and here
    // all map to "Prop_NamedIconPathDeviceOff_String".
    //
    bool bSetupIconUsingExternalResourceFile = true;
    if (!bSetupIconUsingExternalResourceFile) {
      // Setup properties directly in code.
      // Path values are of the form {drivername}\icons\some_icon_filename.png
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceOff_String,
          "{sample}/icons/headset_sample_status_off.png");
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearching_String,
          "{sample}/icons/headset_sample_status_searching.gif");
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer,
          vr::Prop_NamedIconPathDeviceSearchingAlert_String,
          "{sample}/icons/headset_sample_status_searching_alert.gif");
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReady_String,
          "{sample}/icons/headset_sample_status_ready.png");
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReadyAlert_String,
          "{sample}/icons/headset_sample_status_ready_alert.png");
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceNotReady_String,
          "{sample}/icons/headset_sample_status_error.png");
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceStandby_String,
          "{sample}/icons/headset_sample_status_standby.png");
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceAlertLow_String,
          "{sample}/icons/headset_sample_status_ready_low.png");
    }

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
    coordinates.rfBlue[0] = fU;
    coordinates.rfBlue[1] = fV;
    coordinates.rfGreen[0] = fU;
    coordinates.rfGreen[1] = fV;
    coordinates.rfRed[0] = fU;
    coordinates.rfRed[1] = fV;
    return coordinates;
  }

  virtual DriverPose_t GetPose() {
    pose.result = TrackingResult_Running_OK;
    auto resp = remotePoser.returnStatus;
    if (resp != 0) {
      pose.result = TrackingResult_Uninitialized;
    } else {
      pose.vecPosition[0] = remotePoser.newPose[0];
      pose.vecPosition[1] = remotePoser.newPose[1];
      pose.vecPosition[2] = remotePoser.newPose[2];

      pose.qRotation =
          HmdQuaternion_Init(remotePoser.newPose[3], remotePoser.newPose[4],
                             remotePoser.newPose[5], remotePoser.newPose[6]);
    }

    return pose;
  }

  void RunFrame() {
    // In a real driver, this should happen from some pose tracking thread.
    // The RunFrame interval is unspecified and can be very irregular if some
    // other driver blocks it for some periodic task.
    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
      vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
          m_unObjectId, GetPose(), sizeof(DriverPose_t));
    }
  }

  std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
  vr::TrackedDeviceIndex_t m_unObjectId;
  vr::PropertyContainerHandle_t m_ulPropertyContainer;

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

  DriverPose_t pose;
};

//-----------------------------------------------------------------------------
// Purpose:controllerDriver
//-----------------------------------------------------------------------------
class CSampleControllerDriver : public vr::ITrackedDeviceServerDriver {
public:
  CSampleControllerDriver(bool side) {
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

    if (side) {
      m_sSerialNumber = "nut666";
      m_sModelNumber = "MyController666";
    } else {
      m_sSerialNumber = "nut999";
      m_sModelNumber = "MyController999";
    }

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
    handSide_ = side;
  }

  virtual ~CSampleControllerDriver() {}

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
    // as well as what default bindings should be for legacy or other apps
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer, Prop_InputProfilePath_String,
        "{sample}/input/mycontroller_profile.json");

    // create all the input components
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

  virtual DriverPose_t GetPose() {
    poseController.result = TrackingResult_Running_OK;
    auto resp = remotePoser.returnStatus;
    if (resp != 0) {
      poseController.result = TrackingResult_Uninitialized;
    } else {
      int i_indexOffset = 7;
      if (!handSide_) {
        i_indexOffset += 16;
      }
      poseController.vecPosition[0] = remotePoser.newPose[(i_indexOffset + 0)];
      poseController.vecPosition[1] = remotePoser.newPose[(i_indexOffset + 1)];
      poseController.vecPosition[2] = remotePoser.newPose[(i_indexOffset + 2)];

      poseController.qRotation =
          HmdQuaternion_Init(remotePoser.newPose[(i_indexOffset + 3)],
                             remotePoser.newPose[(i_indexOffset + 4)],
                             remotePoser.newPose[(i_indexOffset + 5)],
                             remotePoser.newPose[(i_indexOffset + 6)]);
    }

    return poseController;
  }

  void RunFrame() {
    // Your driver would read whatever hardware state is associated with its
    // input components and pass that in to UpdateBooleanComponent. This could
    // happen in RunFrame or on a thread of your own that's reading USB state.
    // There's no need to update input state unless it changes, but it doesn't
    // do any harm to do so.

    int i_indexOffset = 7;
    if (!handSide_) {
      i_indexOffset += 16;
    }

    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compGrip, remotePoser.newPose[(i_indexOffset + 7)] > 0.1, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compSystem, remotePoser.newPose[(i_indexOffset + 8)] > 0.1, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compAppMenu, remotePoser.newPose[(i_indexOffset + 9)] > 0.1, 0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTrackpadClick, remotePoser.newPose[(i_indexOffset + 10)] > 0.1,
        0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTrackpadTouch, remotePoser.newPose[(i_indexOffset + 14)] > 0.1,
        0);
    vr::VRDriverInput()->UpdateBooleanComponent(
        m_compTriggerClick, remotePoser.newPose[(i_indexOffset + 15)] > 0.1, 0);

    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrigger, remotePoser.newPose[(i_indexOffset + 11)], 0);
    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrackpadX, remotePoser.newPose[(i_indexOffset + 12)], 0);
    vr::VRDriverInput()->UpdateScalarComponent(
        m_compTrackpadY, remotePoser.newPose[(i_indexOffset + 13)], 0);

    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
      vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
          m_unObjectId, GetPose(), sizeof(DriverPose_t));
    }
  }

  void ProcessEvent(const vr::VREvent_t &vrEvent) {
    switch (vrEvent.eventType) {
    case vr::VREvent_Input_HapticVibration: {
      if (vrEvent.data.hapticVibration.componentHandle == m_compHaptic) {
        // This is where you would send a signal to your hardware to trigger
        // actual haptic feedback
        remotePoser.socSend("driver:buzz\n", 12);
        DriverLog("BUZZ!\n");
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
};

//-----------------------------------------------------------------------------
// Purpose: serverDriver
//-----------------------------------------------------------------------------
class CServerDriver_Sample : public IServerTrackedDeviceProvider {
public:
  virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
  virtual void Cleanup();
  virtual const char *const *GetInterfaceVersions() {
    return vr::k_InterfaceVersions;
  }
  virtual void RunFrame();
  virtual bool ShouldBlockStandbyMode() { return true; }
  virtual void EnterStandby() {}
  virtual void LeaveStandby() {}

private:
  CSampleDeviceDriver *m_pNullHmdLatest = nullptr;
  CSampleControllerDriver *m_pRightController = nullptr;
  CSampleControllerDriver *m_pLeftController = nullptr;
};

CServerDriver_Sample g_serverDriverNull;

EVRInitError CServerDriver_Sample::Init(vr::IVRDriverContext *pDriverContext) {
  VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
  remotePoser.socSend("hello\n", 6);

  InitDriverLog(vr::VRDriverLog());

  m_pNullHmdLatest = new CSampleDeviceDriver();
  vr::VRServerDriverHost()->TrackedDeviceAdded(
      m_pNullHmdLatest->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD,
      m_pNullHmdLatest);

  m_pRightController = new CSampleControllerDriver(1);
  m_pLeftController = new CSampleControllerDriver(0);

  vr::VRServerDriverHost()->TrackedDeviceAdded(
      m_pRightController->GetSerialNumber().c_str(),
      vr::TrackedDeviceClass_Controller, m_pRightController);
  vr::VRServerDriverHost()->TrackedDeviceAdded(
      m_pLeftController->GetSerialNumber().c_str(),
      vr::TrackedDeviceClass_Controller, m_pLeftController);

  return VRInitError_None;
}

void CServerDriver_Sample::Cleanup() {
  CleanupDriverLog();
  remotePoser.socClose();
  delete m_pNullHmdLatest;
  delete m_pRightController;
  delete m_pLeftController;
  m_pNullHmdLatest = NULL;
  m_pRightController = NULL;
  m_pLeftController = NULL;
}

void CServerDriver_Sample::RunFrame() {
  // Sleep(16.8);
  int ret = remotePoser.socRecv();

  if (ret == 0) {
    if (m_pNullHmdLatest != NULL) {
      m_pNullHmdLatest->RunFrame();
    }

    if (m_pRightController != NULL) {
      m_pRightController->RunFrame();
    }

    if (m_pLeftController != NULL) {
      m_pLeftController->RunFrame();
    }
  } else if (ret == 10035) {
    // Sleep(3000);
    DriverLog("socket error, trying to reset connection...");
    // remotePoser.socClose();
    // delete .;
    // remotePoser.= new SP::socketPoser(35);
    // remotePoser.socSend("hello\n", 6);
  }
}

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
