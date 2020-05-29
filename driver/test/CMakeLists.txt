include_directories (${TEST_SOURCE_DIR}/src
                     )
find_package(Catch2 CONFIG REQUIRED)
add_executable (Test test.cpp)
target_link_libraries (Test Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(Test)