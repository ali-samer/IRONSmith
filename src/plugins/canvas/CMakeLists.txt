ironsmith_add_plugin(CanvasPlugin
    PLUGIN_NAME Canvas
    PLUGIN_CLASS Canvas::CanvasPlugin

    SOURCES
        CanvasPlugin.hpp
        CanvasPlugin.cpp
        Canvas.json

        CanvasService.hpp
        CanvasService.cpp

        CanvasRenderOptions.hpp
        CanvasViewport.hpp
        CanvasPrimitives.hpp
        CanvasSceneModel.hpp
        CanvasSceneModel.cpp
        CanvasFabricRouter.hpp
        CanvasFabricRouter.cpp
        CanvasLinkRouteEditor.hpp
        CanvasLinkRouteEditor.cpp
        CanvasView.hpp
        CanvasView.cpp

        CanvasControlsPanel.hpp
        CanvasControlsPanel.cpp

    PUBLIC_DEPENDS
        ${QT6_REQUIRED_COMPONENTS}
        IRONSmith::ExtensionSystem
        IRONSmith::Utils
        IRONSmith::CorePlugin
        IRONSmith::DesignModelPlugin
        IRONSmith::CommandPlugin
)

if (BUILD_WITH_TESTS)
    add_subdirectory(tests)
endif ()