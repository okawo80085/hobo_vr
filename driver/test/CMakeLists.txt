cmake_minimum_required(VERSION 3.16)

# Currently there are no useful catch test.
# Most of the C++ work is for OpenVR, which can be tested/debugged through the steamvr log

#set(TEST_SOURCE_DIR ..)
#include_directories (${TEST_SOURCE_DIR}/src
#                     )
find_package(Catch2 CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(Flatbuffers CONFIG REQUIRED)

find_package(pybind11 CONFIG REQUIRED)
add_library (vrsubscriber SHARED test_zmq_pub_sub.cpp ../src/networking/zmqpubsub.cpp ../src/networking/zmqpubsub.h)
target_link_libraries(vrsubscriber PRIVATE pybind11::lto pybind11::embed pybind11::module cppzmq cppzmq-static flatbuffers::flatbuffers)

add_executable(testzmqps test_zmqpubsub.cpp ../src/networking/zmqpubsub.cpp ../src/networking/zmqpubsub.h)
target_link_libraries (testzmqps PRIVATE Catch2::Catch2 cppzmq cppzmq-static flatbuffers::flatbuffers)
include(CTest)
include(Catch)
catch_discover_tests(testzmqps)