cmake_minimum_required(VERSION 3.16)
project(hobovr_driver)

IF (WIN32)
  # Replace this with your vcpkg location
  include($ENV{USERPROFILE}/vcpkg/scripts/buildsystems/vcpkg.cmake)
  find_path(SYSTEM_INCLUDE_DIR openvr_driver.h)
  include_directories(${SYSTEM_INCLUDE_DIR})
endif()

add_subdirectory (src)

enable_testing ()
add_subdirectory (test)