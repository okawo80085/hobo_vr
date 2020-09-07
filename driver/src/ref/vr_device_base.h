#pragma once

#ifndef VR_DEVICE_BASE_H
#define VR_DEVICE_BASE_H

#include "openvr_driver.h"
#include "driverlog.h"

#if defined(_WIN32)
#include "ref/receiver_win.h"

#elif defined(__linux__)
#include "ref/receiver_linux.h"
#define _stricmp strcasecmp

#endif

using namespace vr;


namespace hobovr {
  struct HobovrComponent
  {
    std::string type;
    std::shared_ptr<HobovrExtendedDisplayComponent> component;
  };

  // should be privately inherited
  class HobovrDevice: public vr::ITrackedDeviceServerDriver {
  public:
    HobovrDevice(std::string myserial, std::string deviceBreed, const std::shared_ptr<SockReceiver::DriverReceiver> commSocket=nullptr): m_pBrodcastSocket(commSocket) {
      m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
      m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

      m_sSerialNumber = myserial;
      m_sModelNumber = deviceBreed + m_sSerialNumber;

      if (m_pBrodcastSocket == nullptr) {
        DriverLog("communication socket object is not supplied, this device will not have back communication features(e.g. haptics)");
      }

    }

    virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
      m_unObjectId = unObjectId;
      m_ulPropertyContainer =
          vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, Prop_RenderModelName_String, m_sRenderModelPath.c_str());

      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, Prop_InputProfilePath_String,
          m_sBindPath.c_str());

      if constexpr(m_pBrodcastSocket != nullptr) {
          vr::VRDriverInput()->CreateHapticComponent(m_ulPropertyContainer,
                                                       "/output/haptic", &m_compHaptic);
      }

      return VRInitError_None;
    }

    virtual ~HobovrDevice() {}

    virtual void Deactivate() {
      m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    }

    virtual void EnterStandby() {}

    virtual void PowerOff() {}

    /** debug request from a client */
    virtual void DebugRequest(const char *pchRequest, char *pchResponseBuffer,
                              uint32_t unResponseBufferSize) {
      if (unResponseBufferSize >= 1)
        pchResponseBuffer[0] = 0;
    }

    virtual DriverPose_t GetPose() { return m_Pose; }

    void *GetComponent(const char *pchComponentNameAndVersion) {
      // TODO: this

      return NULL;
    }

    std::string GetSerialNumber() const { return m_sSerialNumber; }

    void ProcessEvent(const vr::VREvent_t &vrEvent) {
      switch (vrEvent.eventType) {
        if constexpr(m_pBrodcastSocket != nullptr)
        {
          case vr::VREvent_Input_HapticVibration: {
            if (vrEvent.data.hapticVibration.componentHandle == m_compHaptic) {
              // haptic!
                m_pBrodcastSocket->send2((m_sSerialNumber + "\n").c_str());
            }
          } break;
        }
      }
    }

    template <typename Number>
    virtual void RunFrame(std::vector<Number> &lastRead) = 0;

  public:
    // openvr api stuff
    vr::TrackedDeviceIndex_t m_unObjectId;
    vr::PropertyContainerHandle_t m_ulPropertyContainer;

    std::string m_sSerialNumber;
    std::string m_sModelNumber;
    std::string m_sRenderModelPath;

    std::string m_sBindPath;

    DriverPose_t m_Pose;

    vr::VRInputComponentHandle_t m_compHaptic;

    std::vector<HobovrComponent> m_vComponents;

    // not openvr api stuff
    std::shared_ptr<SockReceiver::DriverReceiver> m_pBrodcastSocket;
  }
}

#endif // VR_DEVICE_BASE_H