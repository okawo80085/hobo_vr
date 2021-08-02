cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME driver_hobovr)
set(CMAKE_CXX_STANDARD 17)

file(GLOB driver_hobovr_src
  "src/*.cpp"
  "src/*.cc"
  "src/*.h"
  "src/*.hpp"
)

add_library(${TARGET_NAME} SHARED
  ${driver_hobovr_src}
)

add_definitions(-DDRIVER_HOBOVR_EXPORTS)

target_link_libraries(${TARGET_NAME}
  ${OPENVR_LIBRARIES}
  ${CMAKE_DL_LIBS}
)
