# building
## pre build
clone [openvr](https://github.com/ValveSoftware/openvr) and copy `openvr/headers/openvr_driver.h` to `src/`

## windows
building the driver really comes down to 2 steps, setting up cmake and relocating the compiled binaries to their appropriate locations. [cmake](https://cmake.org/) can be found [here](https://cmake.org/download/), set it up for your system and add it to path if you didn't already, also setup visual studio or any other c++ compiler(if you didn't already)

now to actually build the thing, you'll have to build it twice btw, you'll have to run a few commands
```bash
# these 2 command will setup 2 driver builds, a 64 bit one and a 32 bit one, yes for steamvr you need both

cmake -G "Visual Studio 16 2019" -A Win32 . -B "build32"  # 32 bit
cmake -G "Visual Studio 16 2019" -A x64 . -B "build64"    # 64 bit

# now lets build both

cmake --build build32 --config Release
cmake --build build64 --config Release
```
[more info on these cmake commands](https://stackoverflow.com/a/28370892/10190971)

now after the build is done move the 32 bit binary into `hobovr/bin/win32` and the 64 bit binary into `hobovr/bin/win64`

congrats, you're done, now when you start steamvr it'll load your build of `hobo_vr`

## linux
technically steamvr requires 2 binaries, a 32 bit one(for light tasks like watchdog, etc.) and a 64 bit one(the actual driver), but
```
since I'm lazy and its a pain to set it up to work with both linux and windows here is a work around
            - Okawo, famous last words
```
build it like normal with cmake
```bash
# make a directory for the build
mkdir build

# init cmake
cmake ..

# build

cmake --build .
```

then rename `libdriver_hobovr.so` to `driver_hobovr.so` and copy it to both `hobovr/bin/linux32` *and* `hobovr/bin/linux64`

congrats, you're done, now when you start steamvr it'll load your build of `hobo_vr`
