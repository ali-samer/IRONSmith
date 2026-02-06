ironsmith_add_plugin(DesignModelPlugin
        PLUGIN_NAME designmodel
        PLUGIN_CLASS DesignModel::DesignModelPlugin

        SOURCES
        #-== DesignModel Core Components ==-#
            DesignModelGlobal.hpp
            DesignId.hpp
            DesignId.cpp
            DesignSchemaVersion.hpp
            DesignSchemaVersion.cpp
            DesignMetadata.hpp
            DesignMetadata.cpp
            DesignEntities.hpp
            DesignDocument.hpp
            DesignDocument.cpp
            DesignIndex.hpp
            DesignIndex.cpp
            DesignExtras.hpp
            DesignModel.hpp
            DesignModelPlugin.hpp
            DesignModelPlugin.cpp
            DesignModel.json

        #-== Tile, Block, and Links ==-#
            Tile.hpp
            Tile.cpp

        PUBLIC_DEPENDS
            ${QT6_REQUIRED_COMPONENTS}
            IRONSmith::ExtensionSystem
            IRONSmith::Utils
)

if (BUILD_WITH_TESTS)
    add_subdirectory(tests)
endif ()
