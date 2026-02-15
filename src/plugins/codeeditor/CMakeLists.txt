# SPDX-FileCopyrightText: 2026 Samer Ali
# SPDX-License-Identifier: GPL-3.0-only

ironsmith_add_plugin(CodeEditorPlugin
        PLUGIN_NAME CodeEditorPlugin
        PLUGIN_CLASS CodeEditor::Internal::CodeEditorPlugin

        SOURCES
        #-== CodeEditor Core ==-#
            CodeEditor.json
            CodeEditorGlobal.hpp
            CodeEditorPlugin.cpp
            Constants.hpp

        #-== CodeEditor Public APIs ==-#
            api/CodeEditorTypes.hpp
            api/ICodeEditorService.hpp

        #-== CodeEditor Internal Implementations ==-#
            internal/CodeEditorServiceImpl.hpp
            internal/CodeEditorServiceImpl.cpp

        PRIVATE_DEPENDS
            ${QT6_REQUIRED_COMPONENTS}
            IRONSmith::Utils
            IRONSmith::ExtensionSystem
            IRONSmith::CorePlugin
)

if (BUILD_WITH_TESTS)
    add_subdirectory(tests)
endif ()