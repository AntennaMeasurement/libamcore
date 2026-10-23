if(NOT DEFINED FIELD_TEST OR NOT DEFINED FIELD_ROOT)
    message(FATAL_ERROR "FIELD_TEST and FIELD_ROOT are required")
endif()
if(APPLE)
    set(database "${FIELD_ROOT}/Library/Application Support/libamcore/libamcore.db")
    get_filename_component(database_directory "${database}" DIRECTORY)
else()
    set(database "${FIELD_ROOT}/libamcore/libamcore.db")
    get_filename_component(database_directory "${database}" DIRECTORY)
endif()
file(MAKE_DIRECTORY "${database_directory}")
file(REMOVE "${database}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "LOCALAPPDATA=${FIELD_ROOT}" "XDG_DATA_HOME=${FIELD_ROOT}" "HOME=${FIELD_ROOT}"
        "${FIELD_TEST}" "${database}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
string(REGEX MATCHALL
    "amcore (telemetry|operation logging): invalid operation field descriptors"
    diagnostics "${error}")
list(LENGTH diagnostics diagnostic_count)
string(REGEX REPLACE
    "amcore (telemetry|operation logging): invalid operation field descriptors\r?\n"
    "" unexpected "${error}")
string(REGEX REPLACE
    "amcore telemetry: cannot (allocate|serialize) calculation event\r?\n"
    "" unexpected "${unexpected}")
if(NOT result STREQUAL "0" OR NOT unexpected STREQUAL "" OR
   diagnostic_count LESS 4)
    message(FATAL_ERROR "Generic field tests failed: ${result}: ${output}${error}")
endif()
