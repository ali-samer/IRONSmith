ironsmith_add_plugin(CommandPlugin
    PLUGIN_NAME Command
    PLUGIN_CLASS Command::CommandPlugin

    SOURCES
        #-== Command Core Components ==-#
        CommandPluginGlobal.hpp
        CommandError.hpp
        CommandResult.hpp
        CommandDispatcher.hpp
        CommandDispatcher.cpp
        CommandTransaction.hpp
        CommandTransaction.cpp
        CommandPlugin.hpp
        CommandPlugin.cpp
        Command.json

        #-== DocumentModel Interface ==-#
        DesignCommand.hpp
        BuiltInCommands.hpp
        BuiltInCommands.cpp
        DeleteCommands.hpp
        DeleteCommands.cpp

    PUBLIC_DEPENDS
        ${QT6_REQUIRED_COMPONENTS}
        IRONSmith::DesignModelPlugin
        IRONSmith::ExtensionSystem
        IRONSmith::Utils
)

if (BUILD_WITH_TESTS)
    add_subdirectory(tests)
endif ()