include(IRONSmithFunctions)

ironsmith_define_tests(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        LINK_LIBS
            IRONSmith::CanvasPlugin
            IRONSmith::DesignModelPlugin
            Qt6::Test
        TEST CanvasTests
            SOURCES
                tst_CanvasSceneModel.cpp
                tst_CanvasFabricRouter.cpp
                tst_CanvasLinkRouteEditor.cpp
                tst_CanvasRouteEditBehavior.cpp
)
