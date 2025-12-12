function(ironsmith_define_tests)
    set(directory_count 0)
    set(test_count 0)
    set(currentArg 0)
    while (currentArg LESS ${ARGC})
        if ("x${ARGV${currentArg}}" STREQUAL "xDIRECTORY")
            math(EXPR currentArg "${currentArg} + 1")
            if (NOT currentArg LESS ${ARGC})
                message(FATAL_ERROR "Expected base directory!")
            endif()

            set(cur_dir ${directory_count})
            math(EXPR directory_count "${directory_count} + 1")
            set(directory_${cur_dir}_name "${ARGV${currentArg}}")
            # We need a single list of sources to get source_group to work nicely.
            set(directory_${cur_dir}_source_list)

            math(EXPR currentArg "${currentArg} + 1")
            while (currentArg LESS ${ARGC})
                if ("x${ARGV${currentArg}}" STREQUAL "xDIRECTORY")
                    break()
                elseif ("x${ARGV${currentArg}}" STREQUAL "xTEST" OR
                        "x${ARGV${currentArg}}" STREQUAL "xBENCHMARK")
                    set(cur_test ${test_count})
                    math(EXPR test_count "${test_count} + 1")

                    set(test_${cur_test}_is_benchmark $<STREQUAL:"x${ARGV${currentArg}}","xBENCHMARK">)

                    math(EXPR currentArg "${currentArg} + 1")
                    if (NOT currentArg LESS ${ARGC})
                        message(FATAL_ERROR "Expected test name!")
                    endif()

                    set(test_${cur_test}_name "${ARGV${currentArg}}")
                    math(EXPR currentArg "${currentArg} + 1")
                    set(test_${cur_test}_directory ${cur_dir})
                    set(test_${cur_test}_content_dir)
                    set(test_${cur_test}_headers)
                    set(test_${cur_test}_sources)
                    set(test_${cur_test}_tag)

                    set(argumentState 0)
                    while (currentArg LESS ${ARGC})
                        if ("x${ARGV${currentArg}}" STREQUAL "xHEADERS")
                            set(argumentState 1)
                        elseif ("x${ARGV${currentArg}}" STREQUAL "xSOURCES")
                            set(argumentState 2)
                        elseif ("x${ARGV${currentArg}}" STREQUAL "xCONTENT_DIR")
                            math(EXPR currentArg "${currentArg} + 1")
                            if (NOT currentArg LESS ${ARGC})
                                message(FATAL_ERROR "Expected content directory name!")
                            endif()
                            set(test_${cur_test}_content_dir "${ARGV${currentArg}}")
                        elseif ("x${ARGV${currentArg}}" STREQUAL "xTEST" OR
                                "x${ARGV${currentArg}}" STREQUAL "xBENCHMARK" OR
                                "x${ARGV${currentArg}}" STREQUAL "xDIRECTORY")
                            break()
                        elseif (argumentState EQUAL 0)
                            if ("x${ARGV${currentArg}}" STREQUAL "xBROKEN")
                                list(APPEND test_${cur_test}_tag "BROKEN")
                            elseif ("x${ARGV${currentArg}}" STREQUAL "xHANGING")
                                list(APPEND test_${cur_test}_tag "HANGING")
                            elseif ("x${ARGV${currentArg}}" STREQUAL "xSLOW")
                                list(APPEND test_${cur_test}_tag "SLOW")
                            elseif ("x${ARGV${currentArg}}" STREQUAL "xWINDOWS_DISABLED")
                                list(APPEND test_${cur_test}_tag "WINDOWS_DISABLED")
                            elseif ("x${ARGV${currentArg}}" STREQUAL "xAPPLE_DISABLED")
                                list(APPEND test_${cur_test}_tag "APPLE_DISABLED")
                            else()
                                message(FATAL_ERROR "Unknown test tag '${ARGV${currentArg}}'!")
                            endif()
                        elseif (argumentState EQUAL 1)
                            list(APPEND test_${cur_test}_headers
                                    "${IRONSMITH_DIR}/${directory_${cur_dir}_name}${ARGV${currentArg}}"
                            )
                        elseif (argumentState EQUAL 2)
                            list(APPEND test_${cur_test}_sources
                                    "${IRONSMITH_DIR}/${directory_${cur_dir}_name}${ARGV${currentArg}}"
                            )
                        else()
                            message(FATAL_ERROR "Unknown argument state!")
                        endif()
                        math(EXPR currentArg "${currentArg} + 1")
                    endwhile()

                    list(APPEND directory_${cur_dir}_source_list
                            ${test_${cur_test}_sources} ${test_${cur_test}_headers})
                else()
                    message(FATAL_ERROR "Unknown argument inside directory '${ARGV${currentArg}}'!")
                endif()
            endwhile()
        else()
            message(FATAL_ERROR "Unknown argument '${ARGV${currentArg}}'!")
        endif()
    endwhile()

    set(cur_dir 0)
    while (cur_dir LESS directory_count)
        source_group("" FILES ${directory_${cur_dir}_source_list})
        math(EXPR cur_dir "${cur_dir} + 1")
    endwhile()

    set(cur_test 0)
    while (cur_test LESS test_count)
        set(cur_test_name ${test_${cur_test}_name})
        set(cur_dir_name ${directory_${test_${cur_test}_directory}_name})
        if ("BROKEN" IN_LIST test_${cur_test}_tag AND NOT IRONSMITH_BUILD_BROKEN_TESTS)
            message("Skipping broken test ${cur_dir_name}${cur_test_name}, enable with IRONSMITH_BUILD_BROKEN_TESTS")
        elseif ("SLOW" IN_LIST test_${cur_test}_tag AND NOT IRONSMITH_BUILD_SLOW_TESTS)
            message("Skipping slow test ${cur_dir_name}${cur_test_name}, enable with IRONSMITH_BUILD_SLOW_TESTS")
        elseif ("HANGING" IN_LIST test_${cur_test}_tag AND NOT IRONSMITH_BUILD_HANGING_TESTS)
            message("Skipping hanging test ${cur_dir_name}${cur_test_name}, enable with IRONSMITH_BUILD_HANGING_TESTS")
        elseif ("WINDOWS_DISABLED" IN_LIST test_${cur_test}_tag AND WIN32 AND NOT IRONSMITH_BUILD_WINDOWS_DISABLED)
            message("Skipping windows disabled test ${cur_dir_name}${cur_test_name}, enable with IRONSMITH_BUILD_WINDOWS_DISABLED")
        elseif ("APPLE_DISABLED" IN_LIST test_${cur_test}_tag AND APPLE AND NOT IRONSMITH_BUILD_APPLE_DISABLED)
            message("Skipping apple disabled test ${cur_dir_name}${cur_test_name}, enable with IRONSMITH_BUILD_APPLE_DISABLED")
        elseif (${test_${cur_test}_is_benchmark} AND NOT IRONSMITH_BUILD_BENCHMARKS)
            message("Skipping benchmark ${cur_dir_name}${cur_test_name}, enable with IRONSMITH_BUILD_BENCHMARKS")
        else()
            add_executable(${cur_test_name}
                    ${test_${cur_test}_headers}
                    ${test_${cur_test}_sources}
            )
            if (NOT ${test_${cur_test}_is_benchmark})
                if (HAVE_CMAKE_GTEST)
                    # If we have CMake's built-in gtest support use it to add each test
                    # function as a separate test.
                    gtest_add_tests(TARGET ${cur_test_name}
                            WORKING_DIRECTORY "${TOP_DIR}"
                            TEST_PREFIX "${cur_test_name}."
                            TEST_LIST test_cases)
                    set_tests_properties(${test_cases} PROPERTIES TIMEOUT 120)
                else()
                    # Otherwise add each test executable as a single test.
                    add_test(
                            NAME ${cur_test_name}
                            COMMAND ${cur_test_name}
                            WORKING_DIRECTORY "${TOP_DIR}"
                    )
                    set_tests_properties(${cur_test_name} PROPERTIES TIMEOUT 120)
                endif()
            endif()
            if (NOT "x${test_${cur_test}_content_dir}" STREQUAL "x")
                # Copy the content directory to the output directory tree so that
                # tests can be run easily from Visual Studio without having to change
                # the working directory for each test individually.
                file(
                        COPY "${IRONSMITH_DIR}/${cur_dir_name}${test_${cur_test}_content_dir}"
                        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/ironsmith/${cur_dir_name}${test_${cur_test}_content_dir}"
                )
                add_custom_command(TARGET ${cur_test_name} POST_BUILD COMMAND
                        ${CMAKE_COMMAND} ARGS -E copy_directory
                        "${IRONSMITH_DIR}/${cur_dir_name}${test_${cur_test}_content_dir}"
                        "$<TARGET_FILE_DIR:${cur_test_name}>/ironsmith/${cur_dir_name}${test_${cur_test}_content_dir}"
                        COMMENT "Copying test content for ${cur_test_name}" VERBATIM
                )
            endif()
            # Strip the tailing test directory name for the folder name.
            string(REPLACE "test/" "" test_dir_name "${cur_dir_name}")
            set_property(TARGET ${cur_test_name} PROPERTY FOLDER "Tests/${test_dir_name}")
            target_link_libraries(${cur_test_name} PRIVATE ironsmith_test_support)
            # TODO: add `apply_ironsmith_compile_options_to_target()` function
            # apply_ironsmith_compile_options_to_target(${cur_test_name})
        endif()
        math(EXPR cur_test "${cur_test} + 1")
    endwhile()
endfunction()