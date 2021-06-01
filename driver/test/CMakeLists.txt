cmake_minimum_required(VERSION 3.16)

# Currently there are no useful catch test.
# Most of the C++ work is for OpenVR, which can be tested/debugged through the steamvr log

#set(TEST_SOURCE_DIR ..)
#include_directories (${TEST_SOURCE_DIR}/src
#                     )
#find_package(Catch2 CONFIG REQUIRED)
#add_executable (Test test.cpp)
#target_link_libraries (Test Catch2::Catch2)

#include(CTest)
#include(Catch)
#catch_discover_tests(Test)