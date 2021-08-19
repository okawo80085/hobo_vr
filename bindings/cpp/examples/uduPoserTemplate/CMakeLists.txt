cmake_minimum_required(VERSION 3.16)


project(hobo_vr)

set(TARGET_NAME uduPoserTemplate)
set(CMAKE_CXX_STANDARD 17)


IF (EXISTS $ENV{USERPROFILE}/vcpkg)
  include($ENV{USERPROFILE}/vcpkg/scripts/buildsystems/vcpkg.cmake)
  find_path(SYSTEM_INCLUDE_DIR openvr_driver.h)
  include_directories(${SYSTEM_INCLUDE_DIR})
endif()

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()


if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Werror)
endif()


file(GLOB poser_src
  "src/*.cpp"
  "src/*.cc"
  "src/*.h"
  "src/*.hpp"
  "../../../virtualreality/*.h"
)

include_directories("../../virtualreality/")

add_executable(${TARGET_NAME} ${poser_src})
target_link_libraries(
	${TARGET_NAME}
	${CMAKE_DL_LIBS}
)