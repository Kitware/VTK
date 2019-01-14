if (FALSE)
cmake_minimum_required(VERSION 2.8.7)
endif ()
project(libogg)

# Required modules
if (FALSE)
include(GNUInstallDirs)
endif ()
include(CheckIncludeFiles)

# Build options
if (FALSE)
option(BUILD_SHARED_LIBS "Build shared library" OFF)
if(APPLE)
    option(BUILD_FRAMEWORK "Build Framework bundle for OSX" OFF)
endif()
else ()
set(BUILD_FRAMEWORK OFF)
endif ()

if (FALSE)
# Extract project version from configure.ac
file(READ configure.ac CONFIGURE_AC_CONTENTS)
string(REGEX MATCH "AC_INIT\\(\\[libogg\\],\\[([0-9]*).([0-9]*).([0-9]*)" DUMMY ${CONFIGURE_AC_CONTENTS})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
else ()
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 3)
set(PROJECT_VERSION_PATCH 3)
endif ()
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# Helper function to get version-info
function(get_version_info result current_var_name age_var_name revision_var_name)
    string(REGEX MATCH "${current_var_name}=([0-9]*)" DUMMY ${CONFIGURE_AC_CONTENTS})
    set(VERSION_INFO_CURRENT ${CMAKE_MATCH_1})

    string(REGEX MATCH "${age_var_name}=([0-9]*)" DUMMY ${CONFIGURE_AC_CONTENTS})
    set(VERSION_INFO_AGE ${CMAKE_MATCH_1})

    string(REGEX MATCH "${revision_var_name}=([0-9]*)" DUMMY ${CONFIGURE_AC_CONTENTS})
    set(VERSION_INFO_REVISION ${CMAKE_MATCH_1})

    math(EXPR VERSION_INFO_CURRENT_MINUS_AGE "${VERSION_INFO_CURRENT} - ${VERSION_INFO_AGE}")

    set(${result} "${VERSION_INFO_CURRENT_MINUS_AGE}.${VERSION_INFO_AGE}.${VERSION_INFO_REVISION}" PARENT_SCOPE)
endfunction()

# Helper function to configure pkg-config files
function(configure_pkg_config_file pkg_config_file_in)
    set(prefix ${CMAKE_INSTALL_PREFIX})
    set(exec_prefix ${CMAKE_INSTALL_FULL_BINDIR})
    set(libdir ${CMAKE_INSTALL_FULL_LIBDIR})
    set(includedir ${CMAKE_INSTALL_FULL_INCLUDEDIR})
    set(VERSION ${PROJECT_VERSION})
    string(REPLACE ".in" "" pkg_config_file ${pkg_config_file_in})
    configure_file(${pkg_config_file_in} ${CMAKE_CURRENT_BINARY_DIR}/${pkg_config_file} @ONLY)
endfunction()

#message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

# Configure config_type.h
check_include_files(inttypes.h INCLUDE_INTTYPES_H)
check_include_files(stdint.h INCLUDE_STDINT_H)
check_include_files(sys/types.h INCLUDE_SYS_TYPES_H)

set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)

configure_file(include/ogg/config_types.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/ogg/config_types.h @ONLY)

set(OGG_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/include/ogg/config_types.h
    include/ogg/ogg.h
    include/ogg/os_types.h
    include/ogg/vtk_ogg_mangle.h
    "${CMAKE_CURRENT_BINARY_DIR}/include/ogg/vtkogg_export.h"
)

set(OGG_SOURCES
    src/bitwise.c
    src/framing.c
)

if (FALSE)
if(MSVC)
    list(APPEND OGG_SOURCES win32/ogg.def)
endif()
endif ()

if(BUILD_FRAMEWORK)
    set(BUILD_SHARED_LIBS TRUE)
endif()

if (FALSE)
include_directories(include ${CMAKE_CURRENT_BINARY_DIR}/include)
add_library(ogg ${OGG_HEADERS} ${OGG_SOURCES})

get_version_info(OGG_VERSION_INFO "LIB_CURRENT" "LIB_AGE" "LIB_REVISION")
set_target_properties(
    ogg PROPERTIES
    SOVERSION ${OGG_VERSION_INFO}
    PUBLIC_HEADER "${OGG_HEADERS}"
)

if(BUILD_FRAMEWORK)
    set_target_properties(ogg PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION ${PROJECT_VERSION}
        MACOSX_FRAMEWORK_IDENTIFIER org.xiph.ogg
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_FRAMEWORK_BUNDLE_VERSION ${PROJECT_VERSION}
        XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
        OUTPUT_NAME Ogg
    )
endif()

configure_pkg_config_file(ogg.pc.in)

install(TARGETS ogg
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ogg
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ogg.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
else ()
vtk_module_add_module(VTK::ogg
    SOURCES ${OGG_SOURCES}
    HEADERS ${OGG_HEADERS}
    HEADERS_SUBDIR "vtkogg/include/ogg")
include(GenerateExportHeader)
generate_export_header(ogg
    EXPORT_MACRO_NAME   vtkogg_EXPORT
    EXPORT_FILE_NAME    include/ogg/vtkogg_export.h)
target_include_directories(ogg
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkogg/include>")
endif ()
