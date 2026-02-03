ironsmith_add_library(Utils
        SOURCES
            Result.hpp

        #-== Environment ==-#
            Environment.hpp
            EnvironmentQtPolicy.hpp
            EnvironmentQtPolicy.cpp

        PUBLIC_DEPENDS
            Qt6::Core # cmake has to be aware of these targets before request to link (i.e., `find_package(...)`)
)

if (BUILD_WITH_TESTS)
    add_subdirectory(tests)
endif ()