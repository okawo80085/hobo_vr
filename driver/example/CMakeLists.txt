cmake_minimum_required(VERSION 3.16)

set(TEST_SOURCE_DIR ..)
include_directories (${TEST_SOURCE_DIR}/src)

add_executable (example_receiver example_receiver.cpp ${TEST_SOURCE_DIR}/src/ref/receiver.h)

target_link_libraries(example_receiver ${OPENVR_LIBRARIES} ${CMAKE_DL_LIBS})
