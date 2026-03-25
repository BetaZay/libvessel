cmake_minimum_required(VERSION 3.10)
project(VesselProtocol VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vendor: patchelf (Native RPATH editor)
include(FetchContent)
FetchContent_Declare(
  patchelf
  GIT_REPOSITORY https://github.com/NixOS/patchelf.git
  GIT_TAG master
)
FetchContent_MakeAvailable(patchelf)

# libvessel (Core Packing SDK)
add_library(vessel STATIC src/libvessel.cpp)
target_include_directories(vessel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Unified CLI Tool
add_executable(vessel-cli src/vessel.cpp)
set_target_properties(vessel-cli PROPERTIES OUTPUT_NAME vsl)
target_link_libraries(vessel-cli PRIVATE vessel)

# vsl-stub: The Embedded Self-Installing Payload Runner
add_executable(vsl-stub src/vsl_stub.cpp)
set_target_properties(vsl-stub PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")

# Define the system data directory for Vessel internals
if(NOT DEFINED VESSEL_DATA_DIR)
    set(VESSEL_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/vessel")
endif()
add_definitions(-DVESSEL_DATA_DIR="${VESSEL_DATA_DIR}")

# Installation rules
install(TARGETS vessel-cli 
        RUNTIME DESTINATION bin)

install(TARGETS vessel
        ARCHIVE DESTINATION lib)

install(TARGETS vsl-stub
        RUNTIME DESTINATION share/vessel)

install(FILES src/vessel.h DESTINATION include)
