# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Modifications Copyright (C) 2025 Samer Ali
# This file contains modifications for a university capstone project.
include_guard(GLOBAL)

include(IRONSmithBrandingProfile)
include(IRONSmithInternalAPI)

# ironsmith_extend_target(target_name
#   [CONDITION expr]
#   [DEPENDS dep1 dep2 ...]
#   [PUBLIC_DEPENDS dep3 dep4 ...]
#   [DEFINES FOO BAR ...]
#   [PUBLIC_DEFINES BAZ ...]
#   [INCLUDES dir1 dir2 ...]
#   [SYSTEM_INCLUDES dir3 dir4 ...]
#   [PUBLIC_INCLUDES dir5 ...]
#   [PUBLIC_SYSTEM_INCLUDES dir6 ...]
#   [SOURCES file1.cpp file2.cpp ...]
#   [EXPLICIT_MOC moc_this.cpp ...]
#   [SKIP_AUTOMOC dont_moc_this.cpp ...]
#   [PRIVATE_COMPILE_OPTIONS ...]
#   [PUBLIC_COMPILE_OPTIONS ...]
# )
function(ironsmith_extend_target target_name)
    set(_opt_args "")
    set(_single_args "")
    set(_multi_args
            CONDITION
            PRIVATE_DEPENDS
            PUBLIC_DEPENDS
            PRIVATE_DEFINES
            PUBLIC_DEFINES
            PRIVATE_INCLUDES
            PUBLIC_INCLUDES
            PRIVATE_SYSTEM_INCLUDES
            PUBLIC_SYSTEM_INCLUDES
            SOURCES
            EXPLICIT_MOC
            SKIP_AUTOMOC
            PRIVATE_COMPILE_OPTIONS
            PUBLIC_COMPILE_OPTIONS
    )

    cmake_parse_arguments(_args "${_opt_args}" "${_single_args}" "${_multi_args}" "${ARGN}")

    if (_args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "ironsmith_extend_target has unparsed arguments: ${_args_UNPARSED_ARGUMENTS}")
    endif ()

    set(_enabled ON)
    if(_args_CONDITION)
        if (NOT ("${_args_CONDITION}"))
            set(_enabled OFF)
        endif ()
    endif ()

    if (NOT _enabled)
        return()
    endif ()

    ironsmith_add_depends(${target_name}
            PUBLIC ${_args_PUBLIC_DEPENDS}
            PRIVATE ${_args_PRIVATE_DEPENDS}
    )

    target_compile_definitions(${target_name}
            PUBLIC  ${_args_PUBLIC_DEFINES}
            PRIVATE ${_args_PRIVATE_DEFINES}
    )

    target_include_directories(${target_name}
            PUBLIC  ${_args_PUBLIC_INCLUDES}
            PRIVATE ${_args_PRIVATE_INCLUDES}
    )

    target_include_directories(${target_name}
            SYSTEM PUBLIC  ${_args_PUBLIC_SYSTEM_INCLUDES}
            SYSTEM PRIVATE ${_args_PRIVATE_SYSTEM_INCLUDES}
    )

    if (_args_SOURCES)
        target_sources(${target_name}
                PRIVATE ${_args_SOURCES}
        )
    endif ()

    foreach (src IN LISTS _args_EXPLICIT_MOC)
        set_source_files_properties(${src} PROPERTIES SKIP_AUTOMOC OFF)
    endforeach ()

    foreach (src IN LISTS _args_SKIP_AUTOMOC)
        set_source_files_properties(${src} PROPERTIES SKIP_AUTOMOC ON)
    endforeach ()

    target_compile_options(${target_name}
            PUBLIC  ${_args_PUBLIC_COMPILE_OPTIONS}
            PRIVATE ${_args_PRIVATE_COMPILE_OPTIONS}
    )
endfunction()

# add_ironsmith_plugin(target_name
#   [SKIP_INSTALL]
#   [INTERNAL_ONLY]
#   [SKIP_PCH]                 # parsed but ignored for now
#   [PLUGIN_NAME name]
#   [PLUGIN_PATH path]
#   [PLUGIN_CLASS class_name]
#   [VERSION ver]
#   [BUILD_DEFAULT ON|OFF]
#   [CONDITION expr...]
#   [DEPENDS ...]
#   [PUBLIC_DEPENDS ...]
#   [DEFINES ...]
#   [PUBLIC_DEFINES ...]
#   [INCLUDES ...]
#   [SYSTEM_INCLUDES ...]
#   [PUBLIC_INCLUDES ...]
#   [PUBLIC_SYSTEM_INCLUDES ...]
#   [SOURCES ...]
#   [EXPLICIT_MOC ...]
#   [SKIP_AUTOMOC ...]
#   [PRIVATE_COMPILE_OPTIONS ...]
#   [PUBLIC_COMPILE_OPTIONS ...]
# )
function(ironsmith_add_plugin target_name)
    set(_opt_args
            SKIP_INSTALL
            INTERNAL_ONLY
            SKIP_PCH
    )
    set(_single_args
            VERSION
            PLUGIN_PATH
            PLUGIN_NAME
            PLUGIN_CLASS
            BUILD_DEFAULT
    )
    set(_multi_args
            CONDITION
            PRIVATE_DEPENDS
            PUBLIC_DEPENDS
            PRIVATE_INCLUDES
            PUBLIC_INCLUDES
            PRIVATE_SYSTEM_INCLUDES
            PUBLIC_SYSTEM_INCLUDES
            SOURCES
            EXPLICIT_MOC
            SKIP_AUTOMOC
            PRIVATE_DEFINES
            PUBLIC_DEFINES
    )

    cmake_parse_arguments(_args "${_opt_args}" "${_single_args}" "${_multi_args}" ${ARGN})

    if (_args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "ironsmith_add_plugin had unparsed arguments: ${_args_UNPARSED_ARGUMENTS}")
    endif ()

    set(_plugin_name "${target_name}")
    if (_args_PLUGIN_NAME)
        set(_plugin_name "${_args_PLUGIN_NAME}")
    endif ()

    set(_plugin_class "${target_name}Plugin")
    if (_args_PLUGIN_CLASS)
        set(_plugin_class "${_args_PLUGIN_CLASS}")
    endif ()

    set(_plugin_version "${IRONSMITH_VERSION}")
    if (_args_VERSION)
        set(_plugin_version "${_args_VERSION}")
    endif ()

    string(TOUPPER "${target_name}" _target_upper)
    set(_build_var "IRONSMITH_BUILD_PLUGIN_${_target_upper}")

    if (DEFINED _args_BUILD_DEFAULT)
        set(_build_default "${_args_BUILD_DEFAULT}")
    else ()
        set(_build_default "${IRONSMITH_BUILD_PLUGINS_BY_DEFAULT}")
    endif ()

    string(TOUPPER "${_build_var}" _build_var_upper)
    if (DEFINED ENV{${_build_var_upper}})
        set(_build_default "$ENV{${_build_var_upper}}")
    endif ()

    if (_args_INTERNAL_ONLY)
        set(${_build_var} "${_build_default}")
    else ()
        set(${_build_var} "${_build_default}" CACHE BOOL
                "Build IRONSmith plugin ${_plugin_name}."
        )
    endif ()

    set(_condition_result ON)
    if (_args_CONDITION)
        if (NOT ${_args_CONDITION})
            set(_condition_result OFF)
        endif ()
    endif ()

    if (_condition_result AND ${_build_var})
        set(_plugin_enabled ON)
    else ()
        set(_plugin_enabled OFF)
    endif ()

    if (NOT _plugin_enabled)
        return()
    endif ()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${target_name}.json.in")
        list(APPEND _args_SOURCES "${target_name}.json.in")
        configure_file("${target_name}.json.in" "${CMAKE_BINARY_DIR}/${IRONSMITH_PLUGIN_INSTALL_DIR}/${target_name}.json")
    endif ()

    if (IRONSMITH_STATIC_BUILD)
        set(_lib_type STATIC)
    else ()
        set(_lib_type SHARED)
    endif ()

    add_library(${target_name} ${_lib_type})
    add_library(IRONSmith::${target_name} ALIAS ${target_name})

    set_target_properties(${target_name} PROPERTIES
            IRONSMITH_PLUGIN_NAME          "${_plugin_name}"
            IRONSMITH_PLUGIN_CLASS_NAME    "${_plugin_class}"
            IRONSMITH_PLUGIN_VERSION       "${_plugin_version}"
    )

    ironsmith_extend_target(${target_name}
            PRIVATE_DEPENDS         ${_args_PRIVATE_DEPENDS}
            PUBLIC_DEPENDS          ${_args_PUBLIC_DEPENDS}
            PRIVATE_DEFINES         ${_args_PRIVATE_DEFINES}
            PUBLIC_DEFINES          ${_args_PUBLIC_DEFINES}
            PRIVATE_INCLUDES        ${_args_PRIVATE_INCLUDES}
            PUBLIC_INCLUDES         ${_args_PUBLIC_INCLUDES}
            PRIVATE_SYSTEM_INCLUDES ${_args_PRIVATE_SYSTEM_INCLUDES}
            PUBLIC_SYSTEM_INCLUDES  ${_args_PUBLIC_SYSTEM_INCLUDES}
            SOURCES                 ${_args_SOURCES}
            EXPLICIT_MOC            ${_args_EXPLICIT_MOC}
            SKIP_AUTOMOC            ${_args_SKIP_AUTOMOC}
            PRIVATE_COMPILE_OPTIONS ${_args_PRIVATE_COMPILE_OPTIONS}
            PUBLIC_COMPILE_OPTIONS  ${_args_PUBLIC_COMPILE_OPTIONS}
    )

    set(_plugin_dir "${IRONSMITH_PLUGIN_INSTALL_DIR}")
    if (_args_PLUGIN_PATH)
        set(_plugin_dir "${_args_PLUGIN_PATH}")
    endif()

    set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME              "${_plugin_name}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_plugin_dir}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_plugin_dir}"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_plugin_dir}"
            CXX_EXTENSIONS OFF
    )

    list(APPEND IRONSMITH_PLUGINS "${target_name}")

    if (NOT _args_SKIP_INSTALL AND NOT IRONSMITH_STATIC_BUILD)
        install(TARGETS ${target_name}
                RUNTIME DESTINATION "${_plugin_dir}" OPTIONAL
                LIBRARY DESTINATION "${_plugin_dir}" OPTIONAL
                ARCHIVE DESTINATION "${_plugin_dir}" OPTIONAL
        )
    endif()
endfunction(ironsmith_add_plugin)

# ironsmith_add_library(name
#   [STATIC] [SHARED] [OBJECT]
#   [DESTINATION dir]
#   [BUILD_DEFAULT ON|OFF]
#   [CONDITION expr]
#   [DEPENDS ...]
#   [PUBLIC_DEPENDS ...]
#   [DEFINES ...]
#   [PUBLIC_DEFINES ...]
#   [INCLUDES ...]
#   [SYSTEM_INCLUDES ...]
#   [PUBLIC_INCLUDES ...]
#   [PUBLIC_SYSTEM_INCLUDES ...]
#   [SOURCES ...]
#   [EXPLICIT_MOC ...]
#   [SKIP_AUTOMOC ...]
#   [PRIVATE_COMPILE_OPTIONS ...]
#   [PUBLIC_COMPILE_OPTIONS ...]
# )
function(ironsmith_add_library target_name)
    set(_opt_args
            STATIC
            SHARED
            OBJECT
            SKIP_PCH       # parsed but unused for now
    )
    set(_single_args
            DESTINATION
            BUILD_DEFAULT
    )
    set(_multi_args
            CONDITION
            PRIVATE_DEPENDS
            PUBLIC_DEPENDS
            PRIVATE_DEFINES
            PUBLIC_DEFINES
            PRIVATE_SYSTEM_INCLUDES
            PUBLIC_SYSTEM_INCLUDES
            PRIVATE_INCLUDES
            PUBLIC_INCLUDES
            SOURCES
            EXPLICIT_MOC
            SKIP_AUTOMOC
            PRIVATE_COMPILE_OPTIONS
            PUBLIC_COMPILE_OPTIONS
    )

    cmake_parse_arguments(_arg "${_opt_args}" "${_single_args}" "${_multi_args}" ${ARGN})

    if (_arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
                "ironsmith_add_library had unparsed arguments: ${_arg_UNPARSED_ARGUMENTS}"
        )
    endif()

    if (NOT DEFINED IRONSMITH_BUILD_LIBRARIES_BY_DEFAULT)
        set(IRONSMITH_BUILD_LIBRARIES_BY_DEFAULT ON CACHE BOOL
                "Build IRONSmith libraries by default.")
    endif()

    string(TOUPPER "${target_name}" _name_upper)
    set(_build_var "IRONSmith_BUILD_LIBRARY_${_name_upper}")

    if (DEFINED _arg_BUILD_DEFAULT)
        set(_build_default "${_arg_BUILD_DEFAULT}")
    else()
        set(_build_default "${IRONSMITH_BUILD_LIBRARIES_BY_DEFAULT}")
    endif()

    if (DEFINED ENV{${_build_var}})
        set(_build_default "$ENV{${_build_var}}")
    endif()

    set(${_build_var} "${_build_default}" CACHE BOOL
            "Build IRONSmith library ${name}.")

    set(_condition_result ON)
    if (_arg_CONDITION)
        if (NOT (${_arg_CONDITION}))
            set(_condition_result OFF)
        endif()
    endif()

    if (_condition_result AND ${_build_var})
        set(_lib_enabled ON)
    else()
        set(_lib_enabled OFF)
    endif()

    if (NOT _lib_enabled)
        return()
    endif()

    if (_arg_OBJECT)
        set(_lib_type OBJECT)
    else()
        if (_arg_STATIC OR (IRONSMITH_STATIC_BUILD AND NOT _arg_SHARED))
            set(_lib_type STATIC)
        else()
            set(_lib_type SHARED)
        endif()
    endif()

    add_library(${target_name} ${_lib_type})
    add_library(IRONSmith::${target_name} ALIAS ${target_name})

    ironsmith_extend_target(${target_name}
            PRIVATE_DEPENDS             ${_arg_PRIVATE_DEPENDS}
            PUBLIC_DEPENDS              ${_arg_PUBLIC_DEPENDS}
            PRIVATE_DEFINES             ${_arg_PRIVATE_DEFINES}
            PUBLIC_DEFINES              ${_arg_PUBLIC_DEFINES}
            PRIVATE_INCLUDES            ${_arg_PRIVATE_INCLUDES}
            PRIVATE_SYSTEM_INCLUDES     ${_arg_PRIVATE_SYSTEM_INCLUDES}
            PUBLIC_INCLUDES             ${_arg_PUBLIC_INCLUDES}
            PUBLIC_SYSTEM_INCLUDES      ${_arg_PUBLIC_SYSTEM_INCLUDES}
            SOURCES                     ${_arg_SOURCES}
            EXPLICIT_MOC                ${_arg_EXPLICIT_MOC}
            SKIP_AUTOMOC                ${_arg_SKIP_AUTOMOC}
            PRIVATE_COMPILE_OPTIONS     ${_arg_PRIVATE_COMPILE_OPTIONS}
            PUBLIC_COMPILE_OPTIONS      ${_arg_PUBLIC_COMPILE_OPTIONS}
    )

    if (DEFINED IRONSMITH_LIBRARY_INSTALL_DIR)
        set(_DESTINATION "${IRONSMITH_LIBRARY_INSTALL_DIR}")
    else()
        set(_DESTINATION "lib")
    endif()

    if (_arg_DESTINATION)
        set(_DESTINATION "${_arg_DESTINATION}")
    endif()

    set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME              "${target_name}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_DESTINATION}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_DESTINATION}"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_DESTINATION}"
            CXX_EXTENSIONS OFF
    )

    list(APPEND IRONSMITH_LIBRARIES "${target_name}")

    install(TARGETS ${target_name}
            RUNTIME DESTINATION "${_DESTINATION}" OPTIONAL
            LIBRARY DESTINATION "${_DESTINATION}" OPTIONAL
            ARCHIVE DESTINATION "${_DESTINATION}" OPTIONAL
    )
endfunction(ironsmith_add_library)