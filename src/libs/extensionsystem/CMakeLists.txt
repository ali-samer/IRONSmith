ironsmith_add_library(ExtensionSystem
        SOURCES
            IPlugin.hpp
            PluginSpec.hpp
            PluginSpec.cpp
            PluginManager.hpp
            PluginManager.cpp

        PUBLIC_DEPENDS
            Qt6::Core
            Utils

        PRIVATE_DEFINES
            EXTENSIONSYSTEM_LIBRARY
)