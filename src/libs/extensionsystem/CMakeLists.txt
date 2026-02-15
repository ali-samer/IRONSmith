# SPDX-FileCopyrightText: 2026 Samer Ali
# SPDX-License-Identifier: GPL-3.0-only

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
