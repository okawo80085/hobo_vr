cmake_minimum_required(VERSION 3.16)

set(TEST_SOURCE_DIR ..)
include_directories (${TEST_SOURCE_DIR}/src
                     )
find_package(Catch2 CONFIG REQUIRED)
add_executable (Test test.cpp)

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)

target_link_libraries (Test Catch2::Catch2 ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY})
target_include_directories(Test PRIVATE 
  ${AVCODEC_INCLUDE_DIR} 
  ${AVFORMAT_INCLUDE_DIR} 
  ${AVUTIL_INCLUDE_DIR} 
  ${AVDEVICE_INCLUDE_DIR}
)

include(CTest)
include(Catch)
catch_discover_tests(Test)