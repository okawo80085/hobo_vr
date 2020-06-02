cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME driver_sample)

add_library(${TARGET_NAME} SHARED
  driverlog.cpp
  driverlog.h
  driver_sample.cpp
  encode.cpp
)

add_definitions(-DDRIVER_SAMPLE_EXPORTS)


# thanks: https://stackoverflow.com/a/50764569/782170

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)


target_include_directories(${TARGET_NAME} PRIVATE 
  ${AVCODEC_INCLUDE_DIR} 
  ${AVFORMAT_INCLUDE_DIR} 
  ${AVUTIL_INCLUDE_DIR} 
  ${AVDEVICE_INCLUDE_DIR}
)

target_link_libraries(${TARGET_NAME}
  ${OPENVR_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${AVFORMAT_LIBRARY} 
  ${AVUTIL_LIBRARY} 
  ${AVDEVICE_LIBRARY}
  ${AVCODEC_LIBRARY} 
)
