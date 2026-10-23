# Last content update tag: v0.1.0
if(NOT DEFINED AMCORE_CLI)
    message(FATAL_ERROR "AMCORE_CLI must identify the CLI executable")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/telemetry_diagnostics.cmake")

function(check_cli expected_status expected_output)
    execute_process(
        COMMAND "${AMCORE_CLI}" ${ARGN}
        RESULT_VARIABLE status
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    amcore_filter_transport_notices(error)
    if(expected_status STREQUAL "success")
        if(NOT status STREQUAL "0" OR NOT error STREQUAL "" OR
           NOT output MATCHES "${expected_output}")
            message(FATAL_ERROR
                "Command '${ARGN}' failed: status=${status}, stdout='${output}', stderr='${error}'")
        endif()
        if(output MATCHES "[\r\n]  (help|version) +")
            message(FATAL_ERROR
                "Help/version must not appear in the subcommands list: '${output}'")
        endif()
    else()
        if(NOT status STREQUAL "1" OR NOT output STREQUAL "" OR
           NOT error MATCHES "${expected_output}")
            message(FATAL_ERROR
                "Command '${ARGN}' did not report the expected error: status=${status}, stdout='${output}', stderr='${error}'")
        endif()
    endif()
endfunction()

# Anchor numeric results to verify stdout contains only a value and newline.
check_cli(success "^1[\r\n]+$" utility wavelength 299792458)
check_cli(success "^0\\.29979245[0-9]*[\r\n]+$" utility wavelength 1e9)
check_cli(success "^1\\.6[0-9]*[\r\n]+$" farfield distance 299792458 0.1)
check_cli(success "^2\\.5[\r\n]+$" farfield distance 299792458 0.5)
check_cli(success "^32[\r\n]+$" farfield distance 299792458 4)
check_cli(success "^6\\.67128190396[0-9]*[\r\n]+$" farfield distance 1e9 1)

check_cli(success "utility" --help)
check_cli(success "farfield" --help)
check_cli(success "Subcommands:")
check_cli(success "Subcommands:" help)
check_cli(success "Subcommands:" -h)
if(AMCORE_OPERATION_LOGGING)
    set(logging_state enabled)
else()
    set(logging_state disabled)
endif()
if(AMCORE_TELEMETRY)
    set(telemetry_state enabled)
else()
    set(telemetry_state disabled)
endif()
foreach(help IN ITEMS help --help -h)
    check_cli(success "Operation logging: ${logging_state}" ${help})
    check_cli(success "Telemetry: ${telemetry_state}" ${help})
    foreach(module IN ITEMS utility farfield)
        check_cli(success "Operation logging: ${logging_state}" ${module} ${help})
        check_cli(success "Telemetry: ${telemetry_state}" ${module} ${help})
    endforeach()
    check_cli(success "Operation logging: ${logging_state}" utility wavelength ${help})
    check_cli(success "Operation logging: ${logging_state}" farfield distance ${help})
    check_cli(success "Telemetry: ${telemetry_state}" utility wavelength ${help})
    check_cli(success "Telemetry: ${telemetry_state}" farfield distance ${help})
endforeach()
foreach(module IN ITEMS utility farfield)
    check_cli(success "Subcommands:" ${module})
    foreach(help IN ITEMS help --help -h)
        check_cli(success "Subcommands:" ${module} ${help})
    endforeach()
    check_cli(failure "unknown subcommand" ${module} unknown)
    check_cli(failure "unknown subcommand" ${module} --help extra)
endforeach()
foreach(help IN ITEMS help --help -h)
    check_cli(success "frequency_hz" utility wavelength ${help})
    check_cli(success "aperture_length_m" farfield distance ${help})
endforeach()

check_cli(failure "Usage:" utility wavelength)
check_cli(failure "Usage:" utility wavelength 1e9 extra)
check_cli(failure "Usage:" utility wavelength --help extra)
check_cli(failure "Usage:" farfield distance)
check_cli(failure "Usage:" farfield distance 1e9)
check_cli(failure "Usage:" farfield distance 1e9 1 extra)
check_cli(failure "frequency_hz" farfield distance --help extra)

foreach(invalid IN ITEMS 0 -1 nan inf -inf abc 1e9Hz 1e999 1e-999)
    check_cli(failure "frequency_hz" utility wavelength ${invalid})
    check_cli(failure "frequency_hz" farfield distance ${invalid} 1)
    check_cli(failure "aperture_length_m" farfield distance 1e9 ${invalid})
endforeach()

check_cli(failure "out of range" utility wavelength 2.2250738585072014e-308)
check_cli(failure "out of range" farfield distance 2.2250738585072014e-308 1)
check_cli(failure "out of range" farfield distance 1e9 1e200)

if(AMCORE_OPERATION_LOGGING)
    if(NOT DEFINED AMCORE_TEST_ROOT)
        message(FATAL_ERROR "AMCORE_TEST_ROOT is required for logging failure tests")
    endif()
    file(MAKE_DIRECTORY "${AMCORE_TEST_ROOT}")
    set(blocker "${AMCORE_TEST_ROOT}/not-a-directory")
    file(WRITE "${blocker}" "not a directory")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "LOCALAPPDATA=${blocker}" "XDG_DATA_HOME=${blocker}" "HOME=${blocker}"
            "${AMCORE_CLI}" utility wavelength 299792458
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT status STREQUAL "0" OR NOT output MATCHES "^1[\r\n]+$" OR
       NOT error MATCHES "amcore operation logging:")
        message(FATAL_ERROR
            "Logging failure changed CLI behavior: status=${status}, stdout='${output}', stderr='${error}'")
    endif()
endif()
