cmake_minimum_required(VERSION 3.27)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# --------------------------------------------------------------------
# FeatureSummary module is used to display enabled/disabled plugins of
# the IRONSMITH tool as final summary in cmake output
# --------------------------------------------------------------------
include(FeatureSummary)

# --------------------------------------------------------------------
# IRONSMITH CMake modules
# --------------------------------------------------------------------
include(IRONSMITHBrandingProfile)
include(IRONSMITHAPI)

if (NOT DEFINED IRONSMITH_ABSTRACT_SUMMARY)
    message(STATUS "IRONSmith Version: ${IRONSMITH_VERSION_DISPLAY}")
    set(IRONSMITH_ABSTRACT_SUMMARY TRUE CACHE INTERNAL "IRONSmith configuration abstract message")
endif ()

project(IRONSMITH VERSION ${IRONSMITH_VERSION} LANGUAGES CXX)
set(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# --------------------------------------------------------------------
# Global Settings
# --------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# --------------------------------------------------------------------
# Qt's Meta-object, Resource, and UI Compilers
# --------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --------------------------------------------------------------------
# Project options
# --------------------------------------------------------------------
option(IRONSMITH_BUILD_TESTS "Build IRONSmith tests" ON)
option(IRONSMITH_BUILD_SANDBOX "Build the sandbox application" ON)
option(IRONSMITH_STATIC_BUILD "(advanced) Build IRONSmith libraries as static. It's discouraged to \"
                            build IRONSmith libraries as static since the ABI is not guaranteed to be stable \"
                            in early development." OFF)

option(IRONSMITH_BUILD_PLUGINS_BY_DEFAULT "(advanced) Build plugins by default" ON)
option(IRONSMITH_BUILD_LIBRARIES_BY_DEFAULT "(advanced) Build libraries by default" ON)

mark_as_advanced(IRONSMITH_STATIC_BUILD IRONSMITH_BUILD_PLUGINS_BY_DEFAULT IRONSMITH_BUILD_LIBRARIES_BY_DEFAULT)

# --------------------------------------------------------------------
# Find Qt (use system-installed Qt; Conan is not used for Qt)
# --------------------------------------------------------------------
find_package(Qt6 REQUIRED
        COMPONENTS Widgets Core Gui OpenGL
)

set(QT_REQUIRED_LIBS
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGL
)

# --------------------------------------------------------------------
# Subdirectories
# --------------------------------------------------------------------
add_subdirectory(src)
if(IRONSMITH_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

if(IRONSMITH_BUILD_TESTS)
    enable_testing()
    include(IRONSMITH_Tests)
endif()

# --------------------------------------------------------------------
# Packaging (CPack)
# --------------------------------------------------------------------
include(cmake/packaging/PackageConfig.cmake)