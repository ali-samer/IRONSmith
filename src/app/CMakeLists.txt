add_executable(ironsmith
        main.cpp
)

target_link_libraries(ironsmith
        PRIVATE
        ${QT6_REQUIRED_COMPONENTS}
        ExtensionSystem
        Utils
        CorePlugin
        HlirCppBridge
        CodeGenBridge
)

set_target_properties(ironsmith PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${IRONSMITH_BUILD_BINDIR}"
        CXX_EXTENSIONS OFF
)

if (TARGET iron_sanitize_flags)
    target_link_libraries(ironsmith PRIVATE iron_sanitize_flags)
endif()

install(TARGETS ironsmith
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)