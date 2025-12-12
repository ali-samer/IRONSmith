# This file organizes the main IRONSmith modules:
# - app/      : main executable(s)
# - libs/     : core libraries (utils, infra, domain)
# - plugins/  : plugin modules (Core, GraphEditor, etc.)

# Core paths for installation (can be refined later)
set(IRONSMITH_BIN_INSTALL_DIR     "bin"                CACHE STRING "Binary install dir")
set(IRONSMITH_LIBRARY_INSTALL_DIR "lib"                CACHE STRING "Library install dir")
set(IRONSMITH_PLUGIN_INSTALL_DIR  "lib/ironsmith/plugins" CACHE STRING "Plugin install dir")
set(IRONSMITH_INCLUDE_INSTALL_DIR "include/ironsmith"     CACHE STRING "Header install dir")

# Expose these to subdirectories
set(IRONSMITH_BIN_INSTALL_DIR     "${IRONSMITH_BIN_INSTALL_DIR}"     PARENT_SCOPE)
set(IRONSMITH_LIBRARY_INSTALL_DIR "${IRONSMITH_LIBRARY_INSTALL_DIR}" PARENT_SCOPE)
set(IRONSMITH_PLUGIN_INSTALL_DIR  "${IRONSMITH_PLUGIN_INSTALL_DIR}"  PARENT_SCOPE)
set(IRONSMITH_INCLUDE_INSTALL_DIR "${IRONSMITH_INCLUDE_INSTALL_DIR}" PARENT_SCOPE)

add_subdirectory(third_party)
add_subdirectory(app)
add_subdirectory(libs)
add_subdirectory(plugins)
# add_subdirectory(tools)  # we can enable this when we have tools