option(USE_CMAKE_GOOGLE_TEST_INTEGRATION "If enabled, use the google test integration included in CMake." ON)

find_package(GMock MODULE REQUIRED)
if (USE_CMAKE_GOOGLE_TEST_INTEGRATION)
    include(GoogleTest OPTIONAL RESULT_VARIABLE HAVE_CMAKE_GTEST)
    enable_testing()
else ()
    set(HAVE_CMAKE_GTEST OFF)
    return()
endif ()

if (NOT DEFINED IRONSMITH_DIR)
    set(IRONSMITH_DIR "${TOP_DIR}")
endif ()

add_library(ironsmith_test_support
        ${TOP_DIR}/tests/common/TestMain.cpp
)

target_compile_definitions(ironsmith_test_support
        PUBLIC
        ${LIBGMOCK_DEFINES}
)

target_include_directories(ironsmith_test_support
        SYSTEM
        PUBLIC
        ${LIBGMOCK_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIRS}
)

#get_property(IRONSMITH_LIBRARIES GLOBAL PROPERTY IRONSMITH_LIBRARIES)
#get_property(IRONSMITH_PLUGINS GLOBAL PROPERTY IRONSMITH_PLUGINS)

target_link_libraries(ironsmith_test_support
    PUBLIC
        ${LIBGMOCK_LIBRARIES}
        ${QT6_REQUIRED_LIBS}
)

include(IRONSmithFunctions)