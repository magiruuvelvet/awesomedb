cmake_minimum_required(VERSION 3.14)

project(awesomedb++)

# Meta and Helper Modules
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}"
    "${PROJECT_SOURCE_DIR}/cmake/modules")

# Ensure we are building out of source
include(MacroEnsureOutOfSourceBuild)
EnsureOutOfSourceBuild()

# Target creation macro
include(CreateTarget)

# Position Independent Code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Shared Libraries Configuration
set(CMAKE_INSTALL_RPATH "${ORIGIN}")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# Enable (strict) C++20 features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release mode flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")

# Generic flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wno-unknown-pragmas -Wno-switch -Wno-unused-command-line-argument")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wno-unknown-pragmas -Wno-switch -Wno-unused-command-line-argument")

# Prevent implicit function declarations and force developers to include all the required header files
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=implicit-function-declaration")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=implicit-function-declaration")

# Debug mode flags
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "Building with development configuration.")
    add_definitions(-DDEBUG_BUILD)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        message(STATUS "LLVM/clang detected. Enabling LLDB debugger tuning...")
        add_definitions("-glldb")
    endif()
endif()

# project source
add_subdirectory(lib)
