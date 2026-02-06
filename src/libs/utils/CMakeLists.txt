ironsmith_add_library(Utils
        SOURCES
            Result.hpp
			Macros.hpp
			ScopeGuard.hpp
			UtilsGlobal.hpp
			StrongId.hpp
			TreeIds.hpp
			TreeIndex.hpp
			TreeChangeSet.hpp
			VirtualPath.hpp
			VirtualPath.cpp
            PathUtils.hpp
            PathUtils.cpp
            contextmenu/ContextMenu.hpp
            contextmenu/ContextMenu.cpp
            ui/SidebarPanelFrame.hpp
            ui/SidebarPanelFrame.cpp
            ui/ColorSwatchButton.hpp
            ui/ColorSwatchButton.cpp
            ui/LabeledSlider.hpp
            ui/LabeledSlider.cpp
            ui/ConfirmationDialog.hpp
            ui/ConfirmationDialog.cpp
            ui/GridSpec.hpp
            ui/GridLayout.hpp
            ui/GridLayout.cpp
            async/AsyncTask.hpp
            filesystem/RecursiveFileSystemWatcher.hpp
            filesystem/RecursiveFileSystemWatcher.cpp
            filesystem/PathPatternMatcher.hpp
            filesystem/PathPatternMatcher.cpp

        #-== Environment ==-#
            Environment.hpp
            EnvironmentQtPolicy.hpp
            EnvironmentQtPolicy.cpp

        PUBLIC_DEPENDS
			${QT6_REQUIRED_COMPONENTS}
            Qt6::Core # cmake has to be aware of these targets before request to link (i.e., `find_package(...)`)
            Qt6::Widgets
)

if (BUILD_WITH_TESTS)
    add_subdirectory(tests)
endif ()
