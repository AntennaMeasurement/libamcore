# Last content update tag: v0.1.0
cmake_minimum_required(VERSION 3.15)

project(libamcore
    VERSION 0.5.1
    DESCRIPTION "Core computational routines for antenna measurements"
    LANGUAGES C
)

option(BUILD_SHARED_LIBS "Build amcore as a shared library" OFF)
option(BUILD_TESTING "Build the amcore test executable" OFF)
option(AMCORE_BUILD_CLI "Build the amcore command-line tool" ON)
option(AMCORE_BUILD_WASM "Build a standalone WebAssembly module with Emscripten" OFF)
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(amcore src/utility.c src/farfield.c)
add_library(amcore::amcore ALIAS amcore)
set_target_properties(amcore PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_EXTENSIONS OFF
)

target_compile_features(amcore PRIVATE c_std_99)
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
    set_target_properties(amcore_wasm PROPERTIES SUFFIX ".wasm")
    target_link_options(amcore_wasm PRIVATE "-sSTANDALONE_WASM=1")
endif()

if(AMCORE_BUILD_CLI)
    add_executable(amcore_cli cli/main.c)
    target_compile_features(amcore_cli PRIVATE c_std_99)
    set_target_properties(amcore_cli PROPERTIES
        OUTPUT_NAME amcore
        ARCHIVE_OUTPUT_NAME amcore_cli
    )
    target_compile_definitions(amcore_cli PRIVATE
        AMCORE_VERSION="${PROJECT_VERSION}")
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
    add_executable(amcore_tests tests/test_amcore.c)
    target_link_libraries(amcore_tests PRIVATE amcore::amcore)
    add_test(NAME amcore_tests COMMAND $<TARGET_FILE:amcore_tests>)

    if(AMCORE_BUILD_CLI)
        add_test(NAME amcore_cli_calculations COMMAND ${CMAKE_COMMAND}
                "-DAMCORE_CLI=$<TARGET_FILE:amcore_cli>"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.cmake
        )
        add_test(NAME amcore_cli_default_help COMMAND $<TARGET_FILE:amcore_cli>)
        set_tests_properties(amcore_cli_default_help PROPERTIES
            PASS_REGULAR_EXPRESSION "Subcommands:"
        )
        add_test(NAME amcore_cli_help COMMAND $<TARGET_FILE:amcore_cli> help)
        set_tests_properties(amcore_cli_help PROPERTIES
            PASS_REGULAR_EXPRESSION "Subcommands:"
        )
        add_test(NAME amcore_cli_help_option COMMAND $<TARGET_FILE:amcore_cli> --help)
        set_tests_properties(amcore_cli_help_option PROPERTIES
            PASS_REGULAR_EXPRESSION "Subcommands:"
        )
        add_test(NAME amcore_cli_version COMMAND $<TARGET_FILE:amcore_cli> version)
        set_tests_properties(amcore_cli_version PROPERTIES
            PASS_REGULAR_EXPRESSION "amcore ${PROJECT_VERSION}"
        )
        add_test(NAME amcore_cli_version_option COMMAND $<TARGET_FILE:amcore_cli> --version)
        set_tests_properties(amcore_cli_version_option PROPERTIES
            PASS_REGULAR_EXPRESSION "amcore ${PROJECT_VERSION}"
        )
        add_test(NAME amcore_cli_unknown_subcommand COMMAND $<TARGET_FILE:amcore_cli> unknown)
        set_tests_properties(amcore_cli_unknown_subcommand PROPERTIES
            WILL_FAIL TRUE
        )
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
