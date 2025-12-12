cmake_minimum_required(VERSION 3.27)

project(ironsmith_core_plugin LANGUAGES CXX)

# This assumes IRONSMITH_PLUGIN_INSTALL_DIR is defined at the top level,
# e.g. "lib/ironsmith/plugins".
set(IRONSMITH_PLUGIN_RUNTIME_DIR "${CMAKE_BINARY_DIR}/${IRONSMITH_PLUGIN_INSTALL_DIR}")

ironsmith_add_plugin(CorePlugin
        PLUGIN_NAME Core
        PRIVATE_DEPENDS
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        IRONSmith::ExtensionSystem
        IRONSmith::NodeEditor
        SOURCES
        core.cpp
        core.hpp
        Core.json
)

# Ensure the JSON metadata is present in the runtime plugin directory
# where PluginManager::setPluginPaths points.
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Core.json
        ${IRONSMITH_PLUGIN_RUNTIME_DIR}/Core.json
        COPYONLY
)