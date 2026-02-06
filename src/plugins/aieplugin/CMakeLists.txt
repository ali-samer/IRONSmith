ironsmith_add_plugin(AiePlugin
        PLUGIN_NAME AiePlugin
        PLUGIN_CLASS Aie::Internal::AiePlugin

        SOURCES
            AiePlugin.cpp
            AieGlobal.hpp
            AieConstants.hpp
            AieService.hpp
            AieService.cpp
            AiePlugin.json
            AieDeviceTopologies.json
            AiePluginResources.qrc
            NpuProfile.hpp
            NpuProfileLoader.hpp
            NpuProfileLoader.cpp
            NpuProfileCanvasMapper.hpp
            NpuProfileCanvasMapper.cpp
            AieCanvasCoordinator.hpp
            AieCanvasCoordinator.cpp
            api/IAieHost.hpp
            state/AiePanelState.hpp
            state/AiePanelState.cpp
            panels/AieToolPanel.hpp
            panels/AieToolPanel.cpp

        PUBLIC_DEPENDS
            ${QT6_REQUIRED_COMPONENTS}
            IRONSmith::Utils

        PRIVATE_DEPENDS
            IRONSmith::ExtensionSystem
            IRONSmith::CorePlugin
            IRONSmith::CanvasPlugin
)

if (BUILD_WITH_TESTS)
    add_subdirectory(tests)
endif()
