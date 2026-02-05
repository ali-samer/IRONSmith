include(IRONSmithFunctions)
ironsmith_define_tests(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        LINK_LIBS
            IRONSmith::CorePlugin
        TEST SidebarModelTest
            SOURCES
                SidebarModelTests.cpp
        TEST NewTestGroup
            SOURCES


)