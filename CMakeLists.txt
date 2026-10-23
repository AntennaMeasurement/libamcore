# Last content update tag: v0.1.0
cmake_minimum_required(VERSION 3.15)

project(libamcore
    VERSION 0.2.0
    DESCRIPTION "Core computational routines for antenna measurements"
    LANGUAGES C
)

option(BUILD_SHARED_LIBS "Build amcore as a shared library" OFF)
option(BUILD_TESTING "Build the amcore test executable" OFF)
option(AMCORE_BUILD_CLI "Build the amcore command-line tool" ON)
option(AMCORE_BUILD_WASM "Build a standalone WebAssembly module with Emscripten" OFF)
option(AMCORE_ENABLE_OPERATION_LOGGING "Store desktop function operations in SQLite" OFF)
option(AMCORE_ENABLE_TELEMETRY "Send calculation events to an HTTP endpoint" OFF)
set(AMCORE_TELEMETRY_ENDPOINT "" CACHE STRING
    "HTTP(S) endpoint used when AMCORE_ENABLE_TELEMETRY is enabled")
set(AMCORE_TELEMETRY_CA_FILE "" CACHE FILEPATH
    "Optional PEM CA bundle for desktop HTTPS telemetry (empty uses the TLS backend's defaults)")
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(AMCORE_DESKTOP_TELEMETRY OFF)

add_library(amcore src/utility.c src/farfield.c)
add_library(amcore::amcore ALIAS amcore)
set_target_properties(amcore PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_EXTENSIONS OFF
)

target_compile_features(amcore PRIVATE c_std_99)
if(AMCORE_ENABLE_OPERATION_LOGGING OR AMCORE_ENABLE_TELEMETRY)
    target_sources(amcore PRIVATE src/operation.c)
endif()
if(AMCORE_ENABLE_OPERATION_LOGGING)
    if(EMSCRIPTEN OR AMCORE_BUILD_WASM OR
       NOT CMAKE_SYSTEM_NAME MATCHES "^(Windows|Linux|Darwin)$")
        message(FATAL_ERROR
            "AMCORE_ENABLE_OPERATION_LOGGING supports desktop Windows, Linux and macOS only")
    endif()
    find_package(SQLite3 3.24 REQUIRED)
    target_sources(amcore PRIVATE src/operation_log.c)
    target_compile_definitions(amcore PRIVATE
        AMCORE_ENABLE_OPERATION_LOGGING
        AMCORE_LOG_PROJECT_NAME="${PROJECT_NAME}"
    )
    target_link_libraries(amcore PRIVATE SQLite::SQLite3)
endif()
if(AMCORE_ENABLE_TELEMETRY)
    if(NOT AMCORE_TELEMETRY_ENDPOINT MATCHES
       "^https?://[A-Za-z0-9.-]+(:[0-9]+)?(/[^ \t\r\n\"\\;#]*)?$")
        message(FATAL_ERROR
            "AMCORE_TELEMETRY_ENDPOINT must be an absolute HTTP(S) URL with a hostname, optional port and path; credentials, fragments and IPv6 literals are not supported")
    endif()
    find_package(cJSON CONFIG REQUIRED)
    target_link_libraries(amcore PRIVATE cjson)

    set(amcore_telemetry_endpoint "${AMCORE_TELEMETRY_ENDPOINT}")
    target_compile_definitions(amcore PRIVATE
        AMCORE_ENABLE_TELEMETRY
        "AMCORE_TELEMETRY_ENDPOINT=\"${amcore_telemetry_endpoint}\"")

    if(EMSCRIPTEN OR AMCORE_BUILD_WASM)
        if(NOT EMSCRIPTEN OR NOT AMCORE_BUILD_WASM)
            message(FATAL_ERROR
                "WebAssembly telemetry requires AMCORE_BUILD_WASM with the Emscripten toolchain")
        endif()
        target_sources(amcore PRIVATE src/telemetry.c src/telemetry_fetch.c)
        target_compile_options(amcore PUBLIC -pthread)
        target_link_options(amcore INTERFACE -pthread -sFETCH=1)
    elseif(CMAKE_SYSTEM_NAME MATCHES "^(Windows|Linux|Darwin)$")
        set(AMCORE_DESKTOP_TELEMETRY ON)
        find_package(Libwebsockets 4.3 CONFIG REQUIRED)
        if(TARGET websockets_shared)
            target_link_libraries(amcore PRIVATE websockets_shared)
        elseif(TARGET websockets)
            target_link_libraries(amcore PRIVATE websockets)
        else()
            message(FATAL_ERROR "Libwebsockets must export websockets_shared or websockets")
        endif()
        if(AMCORE_TELEMETRY_CA_FILE)
            if(NOT EXISTS "${AMCORE_TELEMETRY_CA_FILE}" OR
               IS_DIRECTORY "${AMCORE_TELEMETRY_CA_FILE}")
                message(FATAL_ERROR
                    "AMCORE_TELEMETRY_CA_FILE must name an existing PEM CA bundle")
            endif()
            file(TO_CMAKE_PATH "${AMCORE_TELEMETRY_CA_FILE}"
                amcore_telemetry_ca_file)
            string(REPLACE "\"" "\\\"" amcore_telemetry_ca_file
                "${amcore_telemetry_ca_file}")
            target_compile_definitions(amcore PRIVATE
                "AMCORE_TELEMETRY_CA_FILE=\"${amcore_telemetry_ca_file}\""
            )
        endif()
        target_sources(amcore PRIVATE
            src/telemetry.c
            src/telemetry_libwebsockets.c
        )
    else()
        message(FATAL_ERROR
            "AMCORE_ENABLE_TELEMETRY supports desktop Windows, Linux, macOS, and Emscripten WebAssembly only")
    endif()
endif()
if(BUILD_SHARED_LIBS)
    target_compile_definitions(amcore
        PUBLIC AMCORE_SHARED
        PRIVATE AMCORE_BUILDING_LIBRARY
    )
    set_target_properties(amcore PROPERTIES
        C_VISIBILITY_PRESET hidden
    )
endif()
target_include_directories(amcore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(AMCORE_BUILD_WASM)
    if(NOT EMSCRIPTEN)
        message(FATAL_ERROR
            "AMCORE_BUILD_WASM requires configuring with the Emscripten toolchain")
    endif()

    add_executable(amcore_wasm src/utility.c src/farfield.c)
    set_target_properties(amcore_wasm PROPERTIES
        OUTPUT_NAME amcore
    )
    target_compile_features(amcore_wasm PRIVATE c_std_99)
    target_include_directories(amcore_wasm
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_options(amcore_wasm PRIVATE
        "-Wl,--no-entry"
        "-sEXPORTED_FUNCTIONS=_amcore_free_space_wavelength,_amcore_far_field_distance"
        "-Wl,--export=speed_of_light_m_per_s"
    )
    if(AMCORE_ENABLE_TELEMETRY)
        set_target_properties(amcore_wasm PROPERTIES SUFFIX ".js")
        target_link_libraries(amcore_wasm PRIVATE cjson)
        target_sources(amcore_wasm PRIVATE
            src/operation.c src/telemetry.c src/telemetry_fetch.c)
        target_compile_definitions(amcore_wasm PRIVATE
            AMCORE_ENABLE_TELEMETRY
            "AMCORE_TELEMETRY_ENDPOINT=\"${amcore_telemetry_endpoint}\""
        )
        target_compile_options(amcore_wasm PRIVATE -pthread)
        target_link_options(amcore_wasm PRIVATE
            -pthread -sFETCH=1 -sFETCH_SUPPORT_INDEXEDDB=0
            -sMODULARIZE=1 -sEXPORT_NAME=createAmcore
            "-sEXPORTED_FUNCTIONS=_amcore_free_space_wavelength,_amcore_far_field_distance,_malloc,_free"
            "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,HEAPF64")
    else()
        set_target_properties(amcore_wasm PROPERTIES SUFFIX ".wasm")
        target_link_options(amcore_wasm PRIVATE "-sSTANDALONE_WASM=1")
    endif()
endif()

if(AMCORE_BUILD_CLI)
    add_executable(amcore_cli cli/main.c)
    target_compile_features(amcore_cli PRIVATE c_std_99)
    set_target_properties(amcore_cli PROPERTIES
        OUTPUT_NAME amcore
        ARCHIVE_OUTPUT_NAME amcore_cli
    )
    target_compile_definitions(amcore_cli
        PRIVATE AMCORE_VERSION="${PROJECT_VERSION}"
                AMCORE_CLI_OPERATION_LOGGING=$<BOOL:${AMCORE_ENABLE_OPERATION_LOGGING}>
                AMCORE_CLI_TELEMETRY=$<BOOL:${AMCORE_ENABLE_TELEMETRY}>
    )
    if(AMCORE_ENABLE_TELEMETRY)
        target_compile_definitions(amcore_cli PRIVATE
            "AMCORE_TELEMETRY_ENDPOINT=\"${amcore_telemetry_endpoint}\"")
    endif()
    target_link_libraries(amcore_cli PRIVATE amcore::amcore)
endif()

install(TARGETS amcore
    EXPORT amcoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(AMCORE_BUILD_CLI)
    install(TARGETS amcore_cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
if(AMCORE_BUILD_WASM)
    install(TARGETS amcore_wasm
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    if(AMCORE_ENABLE_TELEMETRY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/amcore.wasm"
            DESTINATION ${CMAKE_INSTALL_LIBDIR})
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/amcore.worker.js"
            DESTINATION ${CMAKE_INSTALL_LIBDIR} OPTIONAL)
    endif()
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT amcoreTargets
    FILE amcoreTargets.cmake
    NAMESPACE amcore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/amcore
)

configure_package_config_file(
    cmake/amcoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/amcoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/amcore
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/amcoreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/amcoreConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/amcoreConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/amcore
)

if(BUILD_TESTING)
    if(AMCORE_ENABLE_TELEMETRY)
        if(EMSCRIPTEN OR NOT AMCORE_TELEMETRY_ENDPOINT MATCHES
           "^http://127\\.0\\.0\\.1:[0-9]+(/[^ ]*)?$")
            message(FATAL_ERROR
                "Telemetry CTest runs require a dedicated loopback endpoint such as http://127.0.0.1:18765/events; use BUILD_TESTING=OFF for production endpoints or WebAssembly")
        endif()
        find_program(AMCORE_NODE_EXECUTABLE NAMES node)
        if(NOT AMCORE_NODE_EXECUTABLE)
            message(FATAL_ERROR "Telemetry integration tests require Node.js")
        endif()
    endif()

    function(amcore_add_test name)
        if(AMCORE_ENABLE_TELEMETRY)
            add_test(NAME ${name} COMMAND "${AMCORE_NODE_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/telemetry_server.cjs"
                "${AMCORE_TELEMETRY_ENDPOINT}" default ${ARGN})
            set_tests_properties(${name} PROPERTIES
                RESOURCE_LOCK amcore_telemetry_endpoint TIMEOUT 120)
        else()
            add_test(NAME ${name} COMMAND ${ARGN})
        endif()
    endfunction()

    add_executable(amcore_tests tests/test_amcore.c)
    target_link_libraries(amcore_tests PRIVATE amcore::amcore)
    amcore_add_test(amcore_tests $<TARGET_FILE:amcore_tests>)

    if(AMCORE_ENABLE_OPERATION_LOGGING OR AMCORE_ENABLE_TELEMETRY)
        add_executable(amcore_operation_field_tests
            tests/test_operation_fields.c src/operation.c)
        target_compile_features(amcore_operation_field_tests PRIVATE c_std_99)
        target_include_directories(amcore_operation_field_tests PRIVATE include src)
        if(AMCORE_ENABLE_OPERATION_LOGGING)
            target_sources(amcore_operation_field_tests PRIVATE src/operation_log.c)
            target_compile_definitions(amcore_operation_field_tests PRIVATE
                AMCORE_ENABLE_OPERATION_LOGGING AMCORE_LOG_PROJECT_NAME="${PROJECT_NAME}")
            target_link_libraries(amcore_operation_field_tests PRIVATE SQLite::SQLite3)
        endif()
        if(AMCORE_ENABLE_TELEMETRY)
            target_link_libraries(amcore_operation_field_tests PRIVATE cjson)
            target_sources(amcore_operation_field_tests PRIVATE src/telemetry.c)
            target_compile_definitions(amcore_operation_field_tests PRIVATE
                AMCORE_ENABLE_TELEMETRY)
        endif()
        add_test(NAME amcore_operation_field_tests COMMAND ${CMAKE_COMMAND}
            "-DFIELD_TEST=$<TARGET_FILE:amcore_operation_field_tests>"
            "-DFIELD_ROOT=${CMAKE_CURRENT_BINARY_DIR}/operation-field-test"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_operation_fields.cmake)
    endif()

    if(AMCORE_ENABLE_OPERATION_LOGGING)
        add_executable(amcore_operation_log_tests tests/test_operation_log.c)
        target_compile_features(amcore_operation_log_tests PRIVATE c_std_99)
        target_link_libraries(amcore_operation_log_tests PRIVATE
            amcore::amcore SQLite::SQLite3)
        if(MINGW)
            target_link_options(amcore_operation_log_tests PRIVATE -municode)
        endif()
        amcore_add_test(amcore_operation_log_tests ${CMAKE_COMMAND}
                "-DLOG_TEST=$<TARGET_FILE:amcore_operation_log_tests>"
                "-DAMCORE_TELEMETRY=${AMCORE_ENABLE_TELEMETRY}"
                "-DLOG_ROOT=${CMAKE_CURRENT_BINARY_DIR}/operation-log-test"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_operation_log.cmake)
    endif()

    if(AMCORE_BUILD_CLI)
        amcore_add_test(amcore_cli_calculations ${CMAKE_COMMAND}
                "-DAMCORE_CLI=$<TARGET_FILE:amcore_cli>"
                "-DAMCORE_OPERATION_LOGGING=${AMCORE_ENABLE_OPERATION_LOGGING}"
                "-DAMCORE_TELEMETRY=${AMCORE_ENABLE_TELEMETRY}"
                "-DAMCORE_TEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cli-test"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.cmake
        )
        amcore_add_test(amcore_cli_default_help $<TARGET_FILE:amcore_cli>)
        set_tests_properties(amcore_cli_default_help PROPERTIES
            PASS_REGULAR_EXPRESSION "Subcommands:"
        )
        amcore_add_test(amcore_cli_help $<TARGET_FILE:amcore_cli> help)
        set_tests_properties(amcore_cli_help PROPERTIES
            PASS_REGULAR_EXPRESSION "Subcommands:"
        )
        amcore_add_test(amcore_cli_help_option $<TARGET_FILE:amcore_cli> --help)
        set_tests_properties(amcore_cli_help_option PROPERTIES
            PASS_REGULAR_EXPRESSION "Subcommands:"
        )
        amcore_add_test(amcore_cli_version $<TARGET_FILE:amcore_cli> version)
        set_tests_properties(amcore_cli_version PROPERTIES
            PASS_REGULAR_EXPRESSION "amcore ${PROJECT_VERSION}"
        )
        amcore_add_test(amcore_cli_version_option $<TARGET_FILE:amcore_cli> --version)
        set_tests_properties(amcore_cli_version_option PROPERTIES
            PASS_REGULAR_EXPRESSION "amcore ${PROJECT_VERSION}"
        )
        amcore_add_test(amcore_cli_unknown_subcommand $<TARGET_FILE:amcore_cli> unknown)
        set_tests_properties(amcore_cli_unknown_subcommand PROPERTIES
            WILL_FAIL TRUE
        )
    endif()
    if(AMCORE_ENABLE_TELEMETRY)
        add_executable(amcore_telemetry_tests tests/test_telemetry.c)
        target_compile_features(amcore_telemetry_tests PRIVATE c_std_99)
        target_link_libraries(amcore_telemetry_tests PRIVATE amcore::amcore)
        foreach(mode IN ITEMS verify locale status redirect disconnect timeout refused)
            add_test(NAME amcore_telemetry_${mode}
                COMMAND "${AMCORE_NODE_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/telemetry_server.cjs"
                    "${AMCORE_TELEMETRY_ENDPOINT}" "${mode}"
                    $<TARGET_FILE:amcore_telemetry_tests> "${mode}")
            set_tests_properties(amcore_telemetry_${mode} PROPERTIES
                RESOURCE_LOCK amcore_telemetry_endpoint TIMEOUT 30)
        endforeach()
        if(AMCORE_BUILD_CLI)
            foreach(mode IN ITEMS status refused)
                add_test(NAME amcore_telemetry_cli_${mode}
                    COMMAND "${AMCORE_NODE_EXECUTABLE}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/tests/telemetry_server.cjs"
                        "${AMCORE_TELEMETRY_ENDPOINT}" "${mode}"
                        $<TARGET_FILE:amcore_cli> utility wavelength 1e9)
                set_tests_properties(amcore_telemetry_cli_${mode} PROPERTIES
                    RESOURCE_LOCK amcore_telemetry_endpoint TIMEOUT 30)
            endforeach()
        endif()
    endif()

    if(AMCORE_ENABLE_OPERATION_LOGGING)
        # Keep enabled-build tests out of the developer's real user data.
        get_property(amcore_test_names DIRECTORY PROPERTY TESTS)
        foreach(amcore_test_name IN LISTS amcore_test_names)
            if(NOT amcore_test_name STREQUAL "amcore_operation_log_tests")
                set_tests_properties(${amcore_test_name} PROPERTIES ENVIRONMENT
                    "LOCALAPPDATA=${CMAKE_CURRENT_BINARY_DIR}/test-user;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-user;HOME=${CMAKE_CURRENT_BINARY_DIR}/test-user")
            endif()
        endforeach()
    endif()
endif()

set(CPACK_PACKAGE_VENDOR "AntennaMeasurement")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
