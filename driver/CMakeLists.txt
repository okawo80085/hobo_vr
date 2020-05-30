cmake_minimum_required(VERSION 3.16)
project(driver_sample)

IF (WIN32)
  # Replace this with your vcpkg location
  include(D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake)
  find_path(SYSTEM_INCLUDE_DIR openvr_driver.h)
  include_directories(${SYSTEM_INCLUDE_DIR})
endif()

add_subdirectory (src)

add_subdirectory (example)

enable_testing ()
add_subdirectory (test)
