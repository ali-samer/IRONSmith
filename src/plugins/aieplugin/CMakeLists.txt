# SPDX-FileCopyrightText: 2026 Samer Ali
# SPDX-License-Identifier: GPL-3.0-only

ironsmith_add_plugin(AiePlugin
        PLUGIN_NAME AiePlugin
        PLUGIN_CLASS Aie::Internal::AiePlugin

        SOURCES
            AiePlugin.cpp
            AieGlobal.hpp
            AieConstants.hpp
            AieStyleCatalog.hpp
            AieStyleCatalog.cpp
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
            state/AieLayoutSettings.hpp
            state/AieLayoutSettings.cpp
            panels/AieToolPanel.hpp
            panels/AieToolPanel.cpp
            panels/AieNewDesignDialog.hpp
            panels/AieNewDesignDialog.cpp
            design/DesignModel.hpp
            design/DesignModel.cpp
            design/DesignBundleLoader.hpp
            design/DesignBundleLoader.cpp
            design/DesignBundleCreator.hpp
            design/DesignBundleCreator.cpp
            design/DesignState.hpp
            design/DesignStateJson.hpp
            design/DesignStateJson.cpp
            design/DesignStateCanvas.hpp
            design/DesignStateCanvas.cpp
            design/DesignPersistenceController.hpp
            design/DesignPersistenceController.cpp
            design/CanvasDocumentImporter.hpp
            design/CanvasDocumentImporter.cpp
            design/DesignOpenController.hpp
            design/DesignOpenController.cpp

        PUBLIC_DEPENDS
            ${QT6_REQUIRED_COMPONENTS}
            IRONSmith::Utils

        PRIVATE_DEPENDS
            IRONSmith::ExtensionSystem
            IRONSmith::CorePlugin
            IRONSmith::CanvasPlugin
            IRONSmith::ProjectExplorerPlugin

        PRIVATE_DEFINES
            AIEPLUGIN_LIBRARY

        PUBLIC_DEFINES
            AIEPLUGIN_BUILD_SHARED=1
)

if (BUILD_WITH_TESTS)
    add_subdirectory(tests)
endif()
