# c++ bindings for hobo_vr posers
## usage
include `virtualreality.h`
then use `hvr::UduPoserTemplate` as a parent class, further documentation coming soon :tm:

## bulding
each example has it's own CMakeLists.txt, so its a normal cmake build
```bash
# create a build directory
mkdir build
cd build

# init cmake
cmake ..
# build
cmake --build .
```