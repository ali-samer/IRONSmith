include(IRONSmithFunctions)
ironsmith_define_tests(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        LINK_LIBS
            IRONSmith::CommandPlugin
            Qt6::Test
        TEST CommandTests
            SOURCES
                tst_CommandDispatcher.cpp
                tst_CommandTransaction.cpp
                tst_DeleteEntitiesCommand.cpp

)
