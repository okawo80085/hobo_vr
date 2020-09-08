cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME driver_hobovr)
set(CMAKE_CXX_STANDARD 17)

add_library(${TARGET_NAME} SHARED
  driverlog.cpp
  driverlog.h
  driver_hobovr.cpp
)

add_definitions(-DDRIVER_HOBOVR_EXPORTS)

target_link_libraries(${TARGET_NAME}
  ${OPENVR_LIBRARIES}
  ${CMAKE_DL_LIBS}
)
