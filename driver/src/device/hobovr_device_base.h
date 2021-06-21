
namespace hobovr {
    // le version
    static const uint32_t k_nHobovrVersionMajor = 1;
    static const uint32_t k_nHobovrVersionMinor = 0;
    static const uint32_t k_nHobovrVersionBuild = 0;
    static const std::string k_sHobovrVersionGG = "Multi Hobo Drifting";

} // namespace hobovr

namespace hobovr {

    // done for simple device management in vectors
    class HobovrDeviceElement : public vr::ITrackedDeviceServerDriver {
    public:
        virtual void ProcessEvent(const vr::VREvent_t& vrEvent) {};
        virtual std::string GetSerialNumber() const { return ""; };
        virtual void UpdateDeviceBatteryCharge() {};
        virtual void CheckForUpdates() {};
        virtual void PowerOff() {};
        virtual void PowerOn() {};
        virtual void UpdateSectionSettings() {};

        virtual void RunFrame(std::vector<float>& trackingPacket) {} // override this
    };

// should be publicly inherited
class HobovrDevice : public HobovrDeviceElement {
public:
    HobovrDevice(std::string myserial, std::string deviceBreed,
        const std::shared_ptr<SockReceiver::DriverReceiver> commSocket = nullptr, bool UseHaptics=false, bool HasBattery=false) : m_pBrodcastSocket(commSocket),
        m_sSerialNumber(myserial) {

        m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
        m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

        m_sModelNumber = deviceBreed + m_sSerialNumber;

        m_fPoseTimeOffset = vr::VRSettings()->GetFloat(k_pch_Hobovr_Section, k_pch_Hobovr_PoseTimeOffset_Float);
        char buff[1024];
        vr::VRSettings()->GetString(k_pch_Hobovr_Section, k_pch_Hobovr_UpdateUrl_String, buff, sizeof(buff));
        m_sUpdateUrl = buff;

        DriverLog("device: created\n");
        DriverLog("device: breed: %s\n", deviceBreed.c_str());
        DriverLog("device: serial: %s\n", m_sSerialNumber.c_str());
        DriverLog("device: model: %s\n", m_sModelNumber.c_str());
        DriverLog("device: pose time offset: %f\n", m_fPoseTimeOffset);

        if (m_pBrodcastSocket == nullptr && UseHaptics)
            DriverLog("communication socket object is not supplied and haptics are enabled, this device will break on back communication requests(e.g. haptics)\n");

        m_Pose.result = TrackingResult_Running_OK;
        m_Pose.poseTimeOffset = (double)m_fPoseTimeOffset;
        m_Pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
        m_Pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
        m_Pose.qRotation = HmdQuaternion_Init(1, 0, 0, 0);
        m_Pose.vecPosition[0] = 0.;
        m_Pose.vecPosition[1] = 0.;
        m_Pose.vecPosition[2] = 0.;
        m_Pose.willDriftInYaw = true;
        m_Pose.deviceIsConnected = true;
        m_Pose.poseIsValid = true;
        m_Pose.shouldApplyHeadModel = false;
    }

    ~HobovrDevice() {
        m_vComponents.clear();
        DriverLog("device: with serial %s yeeted out of existence\n", m_sSerialNumber.c_str());

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

        vr::VRProperties()->SetBoolProperty(
            m_ulPropertyContainer, vr::Prop_DeviceCanPowerOff_Bool, true);

        DriverLog("device: activated\n");
        DriverLog("device: serial: %s\n", m_sSerialNumber.c_str());
        DriverLog("device: render model path: \"%s\"\n", m_sRenderModelPath.c_str());
        DriverLog("device: input binding path: \"%s\"\n", m_sBindPath.c_str());

        if constexpr (UseHaptics) {
            DriverLog("device: haptics added\n");
            vr::VRDriverInput()->CreateHapticComponent(m_ulPropertyContainer,
                "/output/haptic", &m_compHaptic);
        }
        vr::VRProperties()->SetBoolProperty(
            m_ulPropertyContainer, vr::Prop_Identifiable_Bool, UseHaptics);


        vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
            Prop_DeviceProvidesBatteryStatus_Bool, HasBattery);
        if constexpr (HasBattery) {
            m_fDeviceCharge = GetDeviceCharge(m_sSerialNumber);
            vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
                Prop_DeviceIsCharging_Bool, false);
            vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer,
                Prop_DeviceBatteryPercentage_Float,
                m_fDeviceCharge);
            DriverLog("device: has battery, current charge: %.3f", m_fDeviceCharge * 100);
        }

        vr::VRProperties()->SetStringProperty(
            m_ulPropertyContainer, Prop_Firmware_ManualUpdateURL_String,
            m_sUpdateUrl.c_str());

        bool shouldUpdate = checkForDeviceUpdates(m_sSerialNumber);

        if (shouldUpdate)
            DriverLog("device: update available!\n");

        vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
            Prop_Firmware_UpdateAvailable_Bool, shouldUpdate);
        vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
            Prop_Firmware_ManualUpdate_Bool, shouldUpdate);

        return VRInitError_None;
    }

    virtual void PowerOff() {
        // signal device is "aliven't"
        m_Pose.poseIsValid = false;
        m_Pose.deviceIsConnected = false;
        if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
            vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
                m_unObjectId, m_Pose, sizeof(DriverPose_t));
        }
        DriverLog("device: '%s' disconnected", m_sSerialNumber.c_str());
    }

    virtual void PowerOn() {
        // signal device is "alive"
        m_Pose.poseIsValid = true;
        m_Pose.deviceIsConnected = true;
        if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
            vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
                m_unObjectId, m_Pose, sizeof(DriverPose_t));
        }
        DriverLog("device: '%s' connected", m_sSerialNumber.c_str());
    }

    virtual void Deactivate() {
        DriverLog("device: \"%s\" deactivated\n", m_sSerialNumber.c_str());
        PowerOff();
        m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    }

    virtual void EnterStandby() {}

    /* debug request from a client, TODO: uh... actually implement this? */
    virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer,
        uint32_t unResponseBufferSize) {
        DriverLog("device: \"%s\" got a debug request: \"%s\"", m_sSerialNumber.c_str(), pchRequest);
        if (unResponseBufferSize >= 1)
            pchResponseBuffer[0] = 0;
    }

    virtual vr::DriverPose_t GetPose() { return m_Pose; }

    void* GetComponent(const char* pchComponentNameAndVersion) {
        DriverLog("device: \"%s\" got a request for \"%s\" component\n", m_sSerialNumber.c_str(), pchComponentNameAndVersion);
        for (auto& i : m_vComponents) {
            if (!_stricmp(pchComponentNameAndVersion, i.componentNameAndVersion)) {
                DriverLog("component found, responding...\n");
                switch (i.compType) {
                case EHobovrCompType::EHobovrComp_ExtendedDisplay:
                    return std::get<std::shared_ptr<HobovrExtendedDisplayComponent>>(i.compHandle).get();

                case EHobovrCompType::EHobovrComp_DriverDirectMode:
                    return std::get<std::shared_ptr<HobovrDriverDirectModeComponent>>(i.compHandle).get();

                case EHobovrCompType::EHobovrComp_Camera:
                    return std::get<std::shared_ptr<HobovrCameraComponent>>(i.compHandle).get();

                case EHobovrCompType::EHobovrComp_VirtualDisplay:
                    return std::get<std::shared_ptr<HobovrVirtualDisplayComponent>>(i.compHandle).get();

                }
            }
        }
        DriverLog("component not found, request ignored\n");

        return NULL;
    }

    std::string GetSerialNumber() const { return m_sSerialNumber; }

    void ProcessEvent(const vr::VREvent_t& vrEvent) {
        if constexpr (UseHaptics)
        {
            if (vrEvent.eventType == vr::VREvent_Input_HapticVibration) {
                if (vrEvent.data.hapticVibration.componentHandle == m_compHaptic) {
                    // haptic!
                    m_pBrodcastSocket->send2((m_sSerialNumber + ',' +
                        std::to_string(vrEvent.data.hapticVibration.fDurationSeconds) + ',' +
                        std::to_string(vrEvent.data.hapticVibration.fFrequency) + ',' +
                        std::to_string(vrEvent.data.hapticVibration.fAmplitude) + "\n").c_str());
                }
            }
        }
        switch (vrEvent.eventType) {
        case vr::VREvent_OtherSectionSettingChanged: {
            // handle component settings update
            for (auto& i : m_vComponents) {
                switch (i.compType) {
                case EHobovrCompType::EHobovrComp_ExtendedDisplay:
                    std::get<std::shared_ptr<HobovrExtendedDisplayComponent>>(i.compHandle)->ReloadSectionSettings();
                    break;

                case EHobovrCompType::EHobovrComp_DriverDirectMode:
                    std::get<std::shared_ptr<HobovrDriverDirectModeComponent>>(i.compHandle)->ReloadSectionSettings();
                    break;

                case EHobovrCompType::EHobovrComp_Camera:
                    std::get<std::shared_ptr<HobovrCameraComponent>>(i.compHandle)->ReloadSectionSettings();
                    break;

                case EHobovrCompType::EHobovrComp_VirtualDisplay:
                    std::get<std::shared_ptr<HobovrVirtualDisplayComponent>>(i.compHandle)->ReloadSectionSettings();
                    break;

                }
            }
            // handle device settings update
            UpdateSectionSettings();
            // DriverLog("device '%s': section settings changed", m_sSerialNumber.c_str());
        } break;
        }
    }

    void UpdateDeviceBatteryCharge() {
        if constexpr (HasBattery) {
            float fNewCharge = GetDeviceCharge(m_sSerialNumber);

            if (fNewCharge != m_fDeviceCharge) {
                m_fDeviceCharge = fNewCharge;
                vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer,
                    Prop_DeviceBatteryPercentage_Float, m_fDeviceCharge);
                DriverLog("device: \"%s\" battery charge updated: %f", m_sSerialNumber, m_fDeviceCharge);
            }


            bool bNewIsCharging = ManageDeviceCharging(m_sSerialNumber);
            if (bNewIsCharging != m_bDeviceIsCharging) {
                m_bDeviceIsCharging = bNewIsCharging;
                vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
                    Prop_DeviceIsCharging_Bool, m_bDeviceIsCharging);
                DriverLog("device: \"%s\" is charging: %d", m_bDeviceIsCharging);
            }
        }
    }

    void CheckForUpdates() {
        bool shouldUpdate = checkForDeviceUpdates(m_sSerialNumber);
        vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
            Prop_Firmware_UpdateAvailable_Bool, shouldUpdate);
        vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer,
            Prop_Firmware_ManualUpdate_Bool, shouldUpdate);
    }

protected:
    // openvr api stuff
    vr::TrackedDeviceIndex_t m_unObjectId; // DO NOT TOUCH THIS, parent will handle this, use it as read only!
    vr::PropertyContainerHandle_t m_ulPropertyContainer; // THIS EITHER, use it as read only!


    std::string m_sRenderModelPath; // path to the device's render model, should be populated in the constructor of the derived class
    std::string m_sBindPath; // path to the device's input bindings, should be populated in the constructor of the derived class

    vr::DriverPose_t m_Pose; // device's pose, use this at runtime

    std::vector<HobovrComponent_t> m_vComponents; // components that this device has, should be populated in the constructor of the derived class

    float m_fPoseTimeOffset; // time offset of the pose, set trough the config

    // hobovr stuff
    std::shared_ptr<SockReceiver::DriverReceiver> m_pBrodcastSocket;

private:
    // openvr api stuff that i don't trust you to touch
    vr::VRInputComponentHandle_t m_compHaptic; // haptics, used if UseHaptics is true

    float m_fDeviceCharge; // device charge, 0-none, 1-full, only used if HasBattery is true
    bool m_bDeviceIsCharging; // is device charing, 0-no, 1-yes, only used if HasBattery is true

    std::string m_sUpdateUrl; // url to which steamvr will redirect if checkForDeviceUpdates returns true on Activate, set trough the config
    std::string m_sSerialNumber; // steamvr uses this to identify devices, no need for you to touch this after init
    std::string m_sModelNumber; // steamvr uses this to identify devices, no need for you to touch this after init
};
}
