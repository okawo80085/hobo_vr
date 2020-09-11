
## linux

### compile
to compile the driver on linux use this
```
mkdir ./build
cd ./build
cmake .. -DCMAKE_PREFIX_PATH=/opt/Qt/5.6/gcc_64/lib/cmake -DCMAKE_BUILD_TYPE=Release
make -j4
mv ./libdriver_hobovr.so ./driver_hobovr.so
```

### install compiled
then copy `driver_hobovr.so` to `hobovr/bin/linux32` and `hobovr/bin/linux64`

## windows

### compile
just use the `driver_hobovr.vcxproj` file, so far its the only thing that works

### install compiled
move the the 32 bit dll into `hobovr/bin/win32` and 64 bit dll into `hobovr/bin/win64`


# wtf is all of this?!?!?!

## `hobovr_device_base.h`
`hobovr::HobovrDevice<bool UseHaptics>(std::string myserial, std::string deviceBreed, const std::shared_ptr<SockReceiver::DriverReceiver> commSocket=nullptr)`

this thing is supposed to be a base class for any vr device, set `UseHaptics` to `true` if you want the device to have haptics compiled

publicly inherit it, give it a serial number, a device breed(model number will then be `deviceBreed + serialNumber`), oh and give it a socket object pointer if you enabled haptics

it does allot of boilerplate stuff, in fact the only 2 methods that you might want to override are `Activate()`(but still call the parent `Activate` before anything else) and `RunFrame()`(this should update the device's pose `m_Pose`)

oh also you need to supply your own `m_sRenderModelPath` and `m_sBindPath`

another thing it does is manage components, just init a component(it has to be `hobovr::HobovrComponent_t` tho), add it to the `m_vComponents` vector and you can forget about it, just don't override the `GetComponent()` member function

also parent `ProcessEvent()` will process haptic events using that socket object if it is used in a `vr::VREvent_t` processing loop

oh and lasttly, don't put event processing in tracking loops

## `hobovr_components.h`

supported implementations of components are in it, thats it really

rn it only has the `vr::IVRDisplayComponent` implemented

i.e. this thing:
```c++
hobovr::HobovrExtendedDisplayComponent();
```

also here is an example of an hmd device implementation from `hobovr` driver:
```c++

//-----------------------------------------------------------------------------
// Purpose: hmd device implementation
//-----------------------------------------------------------------------------

class HeadsetDriver : public hobovr::HobovrDevice<false> {
public:
  HeadsetDriver(std::string myserial):HobovrDevice(myserial, "hobovr_hmd_m") {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_hmd_mh0"; // hmd model, stored in hobovr/resources/rendermodels/hobovr_hmd_mh0/
    m_sBindPath = "{hobovr}/input/hobovr_hmd_profile.json"; // hmd input binding, stored in hobovr/resources/input/hobovr_hmd_profile.json

    m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(
        k_pch_Hobovr_Section, k_pch_Hobovr_SecondsFromVsyncToPhotons_Float);

    m_flDisplayFrequency = vr::VRSettings()->GetFloat(
        k_pch_Hobovr_Section, k_pch_Hobovr_DisplayFrequency_Float);

    m_flIPD = vr::VRSettings()->GetFloat(k_pch_Hobovr_Section,
                                         k_pch_Hobovr_IPD_Float);

    // external display component created and added
    hobovr::HobovrComponent_t extDisplayComp = {hobovr::THobovrCompType::THobovrComp_ExtendedDisplay, vr::IVRDisplayComponent_Version};
    extDisplayComp.compHandle = std::make_shared<hobovr::HobovrExtendedDisplayComponent>();
    m_vComponents.push_back(extDisplayComp);

    // m_unObjectId, m_ulPropertyContainer, m_sModelNumber, m_sSerialNumber and m_Pose are initialized by the parent
    // parent also logs most of its actions
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

  // hobovr tracking loop is implemented so that the device just gets the final result
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
```