cmake_minimum_required(VERSION 3.16)

# Currently there are no useful catch test.
# Most of the C++ work is for OpenVR, which can be tested/debugged through the steamvr log

#set(TEST_SOURCE_DIR ..)
#include_directories (${TEST_SOURCE_DIR}/src
#                     )
find_package(Catch2 CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

#find_package(pybind11 CONFIG REQUIRED)
#add_library (vrsubscriber SHARED test_zmq_pub_sub.cpp ../src/networking/zmqpubsub.cpp ../src/networking/zmqpubsub.h)
#target_link_libraries(vrsubscriber PRIVATE pybind11::lto pybind11::embed pybind11::module cppzmq cppzmq-static flatbuffers::flatbuffers)

add_executable(test_all test_main.cpp test_server_message_handler.cpp ../src/server/server_message_handler.h ../src/server/zmqreqrep.h ../src/server/zmqreqrep.cpp ../src/server/protobufs/byte_dict.pb.cc ../src/server/protobufs/byte_dict.pb.h ../src/server/protobufs/controller.pb.cc ../src/server/protobufs/controller.pb.h ../src/server/protobufs/devices.pb.cc ../src/server/protobufs/devices.pb.h ../src/server/protobufs/message.pb.cc ../src/server/protobufs/message.pb.h ../src/server/protobufs/pose.pb.cc ../src/server/protobufs/pose.pb.h)
target_link_libraries (test_all PRIVATE Catch2::Catch2 cppzmq cppzmq-static protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite )
include(CTest)
include(Catch)
catch_discover_tests(test_all)