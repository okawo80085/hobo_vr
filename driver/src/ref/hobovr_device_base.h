#pragma once

#ifndef VR_DEVICE_BASE_H
#define VR_DEVICE_BASE_H

#include "hobovr_components.h"

namespace hobovr {
  struct HobovrComponent
  {
    const char* componentNameAndVersion;
    std::shared_ptr<HobovrExtendedDisplayComponent> componentHandle;
  };

  // should be publicly inherited
  template<bool UseHaptics>
  class HobovrDevice: public vr::ITrackedDeviceServerDriver {
  public:
    HobovrDevice(std::string myserial, std::string deviceBreed,
    const std::shared_ptr<SockReceiver::DriverReceiver> commSocket=nullptr): m_pBrodcastSocket(commSocket),
        m_sSerialNumber(myserial) {

      m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
      m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

      m_sModelNumber = deviceBreed + m_sSerialNumber;
      DriverLog("device created\n");
      DriverLog("device breed: %s\n", deviceBreed.c_str());
      DriverLog("device serial: %s\n", m_sSerialNumber.c_str());
      DriverLog("device model: %s\n", m_sModelNumber.c_str());

      if (m_pBrodcastSocket == nullptr && UseHaptics)
        DriverLog("communication socket object is not supplied and haptics are enabled, this device will break on back communication requests(e.g. haptics)\n");

    }

    virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) {
      m_unObjectId = unObjectId;
      m_ulPropertyContainer =
          vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str());
      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, Prop_RenderModelName_String, m_sRenderModelPath.c_str());

      // return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
      vr::VRProperties()->SetUint64Property(m_ulPropertyContainer,
                                            Prop_CurrentUniverseId_Uint64, 2);

      vr::VRProperties()->SetStringProperty(
          m_ulPropertyContainer, Prop_InputProfilePath_String,
          m_sBindPath.c_str());

      DriverLog("device activated\n");
      DriverLog("device render model: %s\n", m_sRenderModelPath.c_str());
      DriverLog("device input binding: %s\n", m_sBindPath.c_str());

      if constexpr(UseHaptics) {
        DriverLog("device haptics added\n");
        vr::VRDriverInput()->CreateHapticComponent(m_ulPropertyContainer,
                                                       "/output/haptic", &m_compHaptic);
      }


      return VRInitError_None;
    }

    virtual void Deactivate() {
      m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
      DriverLog("device with serial %s deactivated\n", m_sSerialNumber.c_str());
    }

    virtual void EnterStandby() {}

    virtual void PowerOff() {}

    /** debug request from a client, TODO: uh... actually implement this? */
    virtual void DebugRequest(const char *pchRequest, char *pchResponseBuffer,
                              uint32_t unResponseBufferSize) {
      if (unResponseBufferSize >= 1)
        pchResponseBuffer[0] = 0;
    }

    virtual vr::DriverPose_t GetPose() { return m_Pose; }

    void *GetComponent(const char *pchComponentNameAndVersion) {
      for (auto &i : m_vComponents) {
        if (!_stricmp(pchComponentNameAndVersion, i.componentNameAndVersion))
          return i.componentHandle.get();
      }

      return NULL;
    }

    std::string GetSerialNumber() const { return m_sSerialNumber; }

    void ProcessEvent(const vr::VREvent_t &vrEvent) {
      if constexpr(UseHaptics)
      {
        switch (vrEvent.eventType) {
          case vr::VREvent_Input_HapticVibration: {
            if (vrEvent.data.hapticVibration.componentHandle == m_compHaptic) {
                // haptic!
                m_pBrodcastSocket->send2((m_sSerialNumber +
                std::to_string(vrEvent.data.hapticVibration.fDurationSeconds) + ',' +
                std::to_string(vrEvent.data.hapticVibration.fFrequency) + ',' +
                std::to_string(vrEvent.data.hapticVibration.fAmplitude) + "\n").c_str());
            }
          } break;
        }
      }
    }

    template<typename Number>
    void RunFrame(std::vector<Number> &trackingPacket) {} // override this

  protected:
    // openvr api stuff
    vr::TrackedDeviceIndex_t m_unObjectId;
    vr::PropertyContainerHandle_t m_ulPropertyContainer;

    std::string m_sSerialNumber;
    std::string m_sModelNumber;
    std::string m_sRenderModelPath; // should be populated in the constructor of the derived class

    std::string m_sBindPath; // path to the device's bindings, should be populated in the constructor of the derived class

    vr::DriverPose_t m_Pose; // device's pose, use this at runtime

    vr::VRInputComponentHandle_t m_compHaptic;

    std::vector<HobovrComponent> m_vComponents; // components that this device has, should be populated in the constructor of the derived class

    // hobovr stuff
    std::shared_ptr<SockReceiver::DriverReceiver> m_pBrodcastSocket;
  };
}

#endif // VR_DEVICE_BASE_H