include(IRONSmithFunctions)
ironsmith_define_tests(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        LINK_LIBS
            IRONSmith::Utils
        TEST EnvironmentTests
            SOURCES
                EnvironmentTests.cpp
)