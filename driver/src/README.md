
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

it does allot of boiler plait stuff, in fact the only 2 methods that you might want to override are `Activate()`(but still call the parent `Activate` before anything else) and `RunFrame()`(this should update the device's pose `m_Pose`)

oh also you need to supply your own `m_sRenderModelPath` and `m_sBindPath`

another thing it does is manage components, just init it a component, add it to the `m_vComponents` vector and you can forget about it, just don't everride the `GetComponent()` member function

also parent `ProcessEvent()` will process haptic events using that socket object if it is used in a `vr::VREvent_t` processing loop

oh and lasttly, don't put event processing in tracking loops

## `hobovr_components.h`

supported implementations of components are in it, thats it really

rn it only has the `vr::IVRDisplayComponent` implemented
