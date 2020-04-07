## generic vr system
here is an oversimplified abstract overview of how most vr systems interact with SteamVR(and work in general)

open the image in a new tab if the text is too small

![simplified_vr_system.svg](/images/simplified_vr_system.svg)

## hobo_vr
the main difference between a generic SteamVR compatible vr system and `hobo_vr`'s system is the absence of real hardware


instead `hobo_vr` spawns a virtual hmd and a pair of virtual controllers(with input profile of vive wands, [more on input profiles](https://github.com/ValveSoftware/openvr/wiki/IVRDriverInput-Overview)) and waits for input commands from the poser script, regardless of what actual hardware is present

open the image in a new tab if the text is too small

![simplified_hobo_vr_system.svg](/images/simplified_hobo_vr_system.svg)