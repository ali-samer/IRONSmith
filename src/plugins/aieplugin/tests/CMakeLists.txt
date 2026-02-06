include(IRONSmithFunctions)

ironsmith_define_tests(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        LINK_LIBS
            IRONSmith::AiePlugin
            ${QT6_REQUIRED_COMPONENTS}
            Qt6::Test
        TEST AiePluginTests
            SOURCES
                NpuProfileLoaderTests.cpp
                NpuProfileCanvasMapperTests.cpp
)
