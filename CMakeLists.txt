cmake_minimum_required(VERSION 3.21)

project(libamcore
    VERSION 0.1.0
    DESCRIPTION "Core computational routines for antenna measurements"
    LANGUAGES C
)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(AMCORE_BUILD_DOCS "Build the HTML API documentation" OFF)

add_library(amcore src/amcore.c)
add_library(amcore::amcore ALIAS amcore)
set_target_properties(amcore PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_compile_features(amcore PUBLIC c_std_99)
target_include_directories(amcore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS amcore
    EXPORT amcoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
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
    add_test(NAME amcore_tests COMMAND amcore_tests)
endif()

if(AMCORE_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
    configure_file(docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HTML API documentation"
        VERBATIM
    )
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
