cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME driver_hobovr)
set(CMAKE_CXX_STANDARD 17)

set(USE_VULKAN true)

set(VULKAN_LIBS)
set(VULKAN_INCLUDES)
set(VULKAN_FILES)
if(USE_VULKAN)
    add_definitions(-DUSING_VULKAN)
    message("-- Using Vulkan")

    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(SDL2 REQUIRED)

    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(SDL_REQUIRED_LIBRARIES ${SDL_REQUIRED_LIBRARIES} SDL2main)
    endif()

    set(VULKAN_LIBS ${VULKAN_LIBRARIES} ${SDL2_LIBRARIES})

    set(VULKAN_INCLUDES ${VULKAN_INCLUDES} ${SDL2_INCLUDE_DIRS})
    set(VULKAN_LIBS ${VULKAN_LIBS} vulkan dl pthread X11 Xxf86vm Xrandr Xi)
    set(VULKAN_FILES ref/VulkanWindow.cpp ref/VulkanWindow.hpp)

    if(CMAKE_HOST_UNIX)
    endif()


endif()


# Check that the steamVR SDK is installed
# (needed to prevent a segfault in OpenVR).
if(CMAKE_HOST_UNIX)
  find_file(OPENVRPATHS openvrpaths.vrpath PATHS $ENV{HOME}/.config/openvr "$ENV{HOME}/Library/Application Support/OpenVR/.openvr")
  if(${OPENVRPATHS} MATCHES OPENVRPATHS-NOTFOUND)
    message(FATAL_ERROR "${OPENVRPATHS} Please install SteamVR SDK to continue..")
  endif()
endif()

include_directories(${VULKAN_INCLUDES} ${OPENVR_INCLUDE_DIR})


add_library(${TARGET_NAME} SHARED
  driverlog.cpp
  driverlog.h
  driver_hobovr.cpp
  ${VULKAN_FILES}
)

set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")

add_definitions(-DDRIVER_HOBOVR_EXPORTS)

target_link_libraries(${TARGET_NAME}
  ${OPENVR_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${VULKAN_LIBS}
)
