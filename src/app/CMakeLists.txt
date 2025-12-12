cmake_minimum_required(VERSION 3.27)

# IRONSmith main application executable
project(ironsmith_app LANGUAGES CXX)

set(IRONSMITH_APP_TARGET ironsmith)

set(IRONSMITH_APP_SOURCES
        main.cpp
)

add_executable(${IRONSMITH_APP_TARGET}
        ${IRONSMITH_APP_SOURCES}
)

# Where plugins are built at configure-time
# e.g. <build>/lib/ironsmith/plugins
set(IRONSMITH_PLUGIN_RUNTIME_DIR
        "${CMAKE_BINARY_DIR}/${IRONSMITH_PLUGIN_INSTALL_DIR}"
)

target_compile_definitions(${IRONSMITH_APP_TARGET}
        PRIVATE
        IRONSMITH_APP_ID="org.ironsmith.app"
        IRONSMITH_PLUGIN_RELATIVE_PATH="${IRONSMITH_PLUGIN_RUNTIME_DIR}"
)

target_link_libraries(${IRONSMITH_APP_TARGET}
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        IRONSmith::ExtensionSystem
)

# Let the app see the libs/ headers (ExtensionSystem, etc.).
# ExtensionSystem itself already exposes its own include dirs via
# ironsmith_add_library, but adding src/ here is convenient for now.
target_include_directories(${IRONSMITH_APP_TARGET}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(${IRONSMITH_APP_TARGET}
        PROPERTIES
        CXX_EXTENSIONS OFF
)

# Install into bin/ (can be refined later)
install(TARGETS ${IRONSMITH_APP_TARGET}
        RUNTIME DESTINATION "${IRONSMITH_BIN_INSTALL_DIR}"
)