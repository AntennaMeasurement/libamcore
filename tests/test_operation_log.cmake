if(NOT DEFINED LOG_TEST OR NOT DEFINED LOG_ROOT)
    message(FATAL_ERROR "LOG_TEST and LOG_ROOT are required")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/telemetry_diagnostics.cmake")

function(run_log_test root argument expect_error)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "LOCALAPPDATA=${root}" "XDG_DATA_HOME=${root}" "HOME=${root}"
            ${ARGN}
            "${LOG_TEST}" "${argument}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    amcore_filter_transport_notices(error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Logging test failed (${result}): ${output}${error}")
    endif()
    if(expect_error)
        if(NOT error MATCHES "amcore operation logging:")
            message(FATAL_ERROR "Missing logging failure diagnostic: ${error}")
        endif()
    elseif(NOT error STREQUAL "")
        message(FATAL_ERROR "Unexpected logging diagnostic: ${error}")
    endif()
endfunction()

if(APPLE)
    set(data_suffix "/Library/Application Support/libamcore")
else()
    set(data_suffix "/libamcore")
endif()

# Exercise recursive directory creation, Unicode paths, and persistence.
string(ASCII 195 188 unicode_utf8)
set(root "${LOG_ROOT}/user ${unicode_utf8}/nested")
set(database "${root}${data_suffix}/libamcore.db")
file(REMOVE "${database}")
run_log_test("${root}" failure FALSE)
run_log_test("${root}" "${database}" FALSE)
run_log_test("${root}" "${database}" FALSE)

if(UNIX AND NOT APPLE)
    set(fallback_root "${LOG_ROOT}/fallback")
    set(fallback_database "${fallback_root}/.local/share/libamcore/libamcore.db")
    file(REMOVE "${fallback_database}")
    run_log_test("${fallback_root}" failure FALSE "XDG_DATA_HOME=relative")
    run_log_test("${fallback_root}" "${fallback_database}" FALSE "XDG_DATA_HOME=")
    run_log_test("${fallback_root}" "${fallback_database}" FALSE "--unset=XDG_DATA_HOME")
endif()

set(blocker "${LOG_ROOT}/not-a-directory")
file(WRITE "${blocker}" "not a directory")
run_log_test("${blocker}" failure TRUE)

set(corrupt_root "${LOG_ROOT}/corrupt")
file(MAKE_DIRECTORY "${corrupt_root}${data_suffix}")
set(corrupt_database "${corrupt_root}${data_suffix}/libamcore.db")
file(WRITE "${corrupt_database}" "not a SQLite database")
run_log_test("${corrupt_root}" failure TRUE)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env --unset=LOCALAPPDATA --unset=XDG_DATA_HOME
        --unset=HOME "${LOG_TEST}" failure
    RESULT_VARIABLE result ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT error MATCHES "amcore operation logging:")
    message(FATAL_ERROR "Missing user directory failure test failed: ${result}: ${error}")
endif()
