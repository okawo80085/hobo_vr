# Hobo VR
hobo_vr is a collection of tools made to easy prototyping of a headset for SteamVR.

hobo_vr tools and features:
* `hobovr` a flexible SteamVR driver(see `driver/` and `hobovr/`)
* `virtualreality` API to communicate with the `hobovr` driver from other languages and processes(see `bindings/`)
* `poser` is a term used to describe a program that implements a tracking system and uses the `virtualreality` API to integrate it's tracking system and devices with SteamVR
* Installers to make the setup process as easy as possible(see `installers/`)
* Linux support, so far hobo_vr is the only open source SteamVR driver solution that works on Linux(more info on the topic can be found below)

Upcoming hobo_vr features and tools:
* New tracking systems
* New lens calibration tools
* New camera calibration tools
* A SteamVR driver update that adds configurable controllers
* New pre-built `posers` bundled with installers to make user experience as easy as possible

# `hobovr` SteamVR driver
There are 2 main features of this driver that distinguish it from the rest.


One is an open and flexible communication protocol which allows developers to fully control the driver without the need to directly use openvr_driver API, more info [here](https://github.com/okawo80085/hobo_vr/wiki/Driver:-communication-protocol), for convenience the said protocol is implemented in `virtualreality`.

And the other is generic device configurability, one can configure devices either at runtime or not, hobo_vr provides 3 types of generic devices at the moment of writing this:
    1. Headset, inputs(tracking) outputs(headset view)
    2. Controllers, inputs(tracking, buttons, trigger, touchpad) outputs(haptic signals)
    3. Tracker, inputs(tracking) outputs(haptic signals)

More info on device types can be found [here](https://github.com/okawo80085/hobo_vr/wiki/Driver:-device-types).


# `virtualreality` API
A simple yet flexible object oriented implementation of the [`hobovr` driver protocol](https://github.com/okawo80085/hobo_vr/wiki/Driver:-communication-protocol) and a runtime for the said protocol, more info can be found [here](https://github.com/okawo80085/hobo_vr/wiki/virtualreality-API).

This API has multiple language implementations, currently Python and C++, see `bindings/`.

# Poser(s)
As mentioned before poser is a term used to describe a program/process/solution that implements the `hobovr` driver protocol(via `virtualreality` API or otherwise) to implement tracking and hw communication,
be it tracking(constellation tracking, SLAM, blob tracking, IMU tracking, etc.) or inputs(button events, trigger readings, touchpad coordinates, joystick location, etc.).

Most extensive poser examples can be found in `bindings/python/examples/`. C++ poser examples are also available of course, but are way less informative and there are fewer of them at the moment of writing this, the said C++ examples can be found in `bindings/cpp/examples/`.

# Why?
Because the alternatives suck and prototyping with openvr_driver API is a huge pain.

# Contributing
Is always welcomed. Join the discussion over at Discord [here](https://discord.gg/PrfUEkC), but pull requests are welcome here too. Please explain the motivation for a given change and examples of its effect.

> Also it is strongly encouraged to follow these [contribution guidelines](https://github.com/okawo80085/hobo_vr/blob/master/CONTRIBUTING.md)

# License
This project falls under the MIT license.