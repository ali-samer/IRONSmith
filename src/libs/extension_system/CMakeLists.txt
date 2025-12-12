# ExtensionSystem: AIECAD's plugin framework library.
# Provides:
#   - ironsmith::IPlugin
#   - ironsmith::PluginSpec
#   - ironsmith::PluginManager
#
# This is a Qt-based library and at minimum needs QtCore.

ironsmith_add_library(ExtensionSystem
        PRIVATE_DEPENDS
            Qt6::Core

        PRIVATE_INCLUDES
            ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC_INCLUDES
            ${CMAKE_CURRENT_SOURCE_DIR}

        SOURCES
            ExtensionSystemGlobal.hpp
            ExtensionSystemGlobal.cpp
            IPlugin.hpp
            IPlugin.cpp
            PluginSpec.hpp
            PluginSpec.cpp
            PluginManager.hpp
            PluginManager.cpp
)