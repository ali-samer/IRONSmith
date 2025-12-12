# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Modifications Copyright (C) 2025 Samer Ali
# This file contains modifications for a university capstone project.

include_guard(GLOBAL)

function(ironsmith_check_disabled_targets target_name dependent_targets)
    foreach (dep IN LISTS ${dependent_targets})
        foreach (type PLUGIN LIBRARY)
            string(TOUPPER "IRONSMITH_BUILD_${type}_${dep}" build_target)
            if (DEFINED ${build_target} AND NOT ${build_target})
                message(SEND_ERROR "Target ${target_name} depends on ${dep} which was "
                        "disabled via ${build_target} set to ${${build_target}}")
            endif ()
        endforeach ()
    endforeach ()
endfunction(ironsmith_check_disabled_targets)

function(ironsmith_add_depends target_name)
    cmake_parse_arguments(_args "" "" "PUBLIC;PRIVATE" ${ARGN})
    if (${_args_UNPARSED_ARGUMENTS})
        message(FATAL_ERROR "ironsmith_add_depends had unparsed arguments: ${_args_UNPARSED_ARGUMENTS}")
    endif ()

    set(deps "${_args_PUBLIC};${_args_PRIVATE}")
    ironsmith_check_disabled_targets(${target_name} deps)

    target_link_libraries(${target_name} PUBLIC ${_args_PUBLIC} PRIVATE ${_args_PRIVATE})
endfunction(ironsmith_add_depends)

