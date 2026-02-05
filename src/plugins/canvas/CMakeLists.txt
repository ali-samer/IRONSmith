ironsmith_add_plugin(CanvasPlugin
        PLUGIN_NAME Canvas
        PLUGIN_CLASS Canvas::CanvasPlugin

        SOURCES
        #-== Core Canvas ==-#
            CanvasGlobal.hpp
            CanvasConstants.hpp
            CanvasTypes.hpp
            CanvasPorts.hpp
            CanvasFabric.hpp
            CanvasFabric.cpp
            CanvasRenderContext.hpp
            CanvasItem.hpp
            CanvasCommand.hpp
            CanvasCommandManager.hpp
            CanvasCommandManager.cpp
            CanvasCommands.hpp
            CanvasCommands.cpp
            CanvasStyle.hpp
            CanvasStyle.cpp
            CanvasBlock.hpp
            CanvasBlock.cpp
            CanvasBlockContent.hpp
            CanvasBlockContent.cpp
            CanvasWire.hpp
            CanvasWire.cpp
            CanvasPlugin.cpp
            CanvasDocument.hpp
            CanvasDocument.cpp
            CanvasView.hpp
            CanvasView.cpp
            CanvasController.hpp
            CanvasController.cpp
            utils/CanvasGeometry.hpp

        #-== Canvas Tools ==-#
            Tools.hpp
            Tools.cpp

        #-== Public APIs ==-#
            api/ICanvasHost.hpp

        #-== Internal Interface Implementations ==-#
            internal/CanvasHostImpl.hpp
            internal/CanvasHostImpl.cpp
            internal/CanvasWireRouting.hpp
            internal/CanvasWireRouting.cpp

        PRIVATE_DEPENDS
            ${QT6_REQUIRED_COMPONENTS}
            IRONSmith::ExtensionSystem
            IRONSmith::Utils
            IRONSmith::CorePlugin
)

if (BUILD_WITH_TESTS)
	add_subdirectory(tests)
endif()
