# driver
this is only the source and building instruction for the `hobovr` driver, the built driver itself is located in `../hobovr/`.

# building
## pre build
make sure that submodules are initialized and are up to date
```bash
git submodule init
git submodule update
```

## windows
building the driver really comes down to 2 steps, setting up cmake and visual studio, because on windows we use visual studio as the generator for cmake. [cmake](https://cmake.org/) can be found [here](https://cmake.org/download/), set it up for your system and add it to path if you didn't already, also setup visual studio and it's C++ tool chain

now to actually build the thing, you'll have to build it twice btw
```bash
# -DINSTALL_X86_DRIVER=1 is very important for 32 bit builds
# because otherwise you risk installing a 32 bit driver into the 64 bit target
cmake -G "Visual Studio 16 2019" -A Win32 . -B "build32" -DINSTALL_X86_DRIVER=1  # 32 bit
cmake -G "Visual Studio 16 2019" -A x64 . -B "build64"    # 64 bit

cmake --build build32 --config Release
cmake --build build64 --config Release

cmake --install build32
cmake --install build64
```

congrats, you're done, now when you start steamvr it'll load your build of `hobovr`

## linux
steamvr requires 2 binaries, a 32 bit one(for light tasks like watchdog, etc.) and a 64 bit one(the actual driver)


build it like normal with cmake, except you need to build it twice, 32 and 64 bit
```bash
# -DINSTALL_X86_DRIVER=1 is very important for 32 bit builds
# because otherwise you risk installing a 32 bit driver into the 64 bit target
cmake . -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_C_FLAGS=-m32 -B "build32" -DINSTALL_X86_DRIVER=1  # 32 bit
cmake . -DCMAKE_CXX_FLAGS=-m64 -DCMAKE_C_FLAGS=-m64 -B "build64"  # 64 bit

cmake --build build32 --config Release
cmake --build build64 --config Release

cmake --install build32
cmake --install build64
```

congrats, you're done, now when you start steamvr it'll load your build of `hobovr`
