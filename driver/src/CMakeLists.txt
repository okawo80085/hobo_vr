cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME driver_hobovr)
#set(CMAKE_CXX_STANDARD 17)

find_package(cppzmq CONFIG REQUIRED)
find_package(Flatbuffers CONFIG REQUIRED)


add_library(${TARGET_NAME} SHARED
  driverlog.cpp
  driverlog.h
  driver_hobovr.cpp
  "server/zmqreqrep.cpp" "device/hobovr_device_base.h" "server/hobovr_server.cpp" "server/hobovr_server.h")

add_definitions(-DDRIVER_HOBOVR_EXPORTS)

target_link_libraries(${TARGET_NAME}
  ${OPENVR_LIBRARIES}
  ${CMAKE_DL_LIBS}
  PRIVATE cppzmq cppzmq-static flatbuffers::flatbuffers
)
