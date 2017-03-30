# This file is copyrighted under the BSD-license for buildsystem files of KDE
# copyright 2010, Patrick Spendrin <ps_ml@gmx.de>

project(expat)

cmake_minimum_required(VERSION 2.6)
set(PACKAGE_BUGREPORT "expat-bugs@libexpat.org")
set(PACKAGE_NAME "expat")
set(PACKAGE_VERSION "2.2.0")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")

if (FALSE) # XXX(kitware): Force build settings.
option(BUILD_tools "build the xmlwf tool for expat library" ON)
option(BUILD_examples "build the examples for expat library" ON)
option(BUILD_tests "build the tests for expat library" ON)
option(BUILD_shared "build a shared expat library" ON)
else ()
set(BUILD_tools OFF)
set(BUILD_examples OFF)
set(BUILD_tests OFF)
set(BUILD_shared ${BUILD_SHARED_LIBS})
endif ()

# configuration options
if (FALSE) # XXX(kitware): Force configuration values.
set(XML_CONTEXT_BYTES 1024 CACHE STRING "Define to specify how much context to retain around the current parse point")
option(XML_DTD "Define to make parameter entity parsing functionality available" ON)
option(XML_NS "Define to make XML Namespaces functionality available" ON)
else ()
set(XML_CONTEXT_BYTES 1024)
set(XML_DTD ON)
set(XML_NS ON)
endif ()

if(XML_DTD)
    set(XML_DTD 1)
else(XML_DTD)
    set(XML_DTD 0)
endif(XML_DTD)
if(XML_NS)
    set(XML_NS 1)
else(XML_NS)
    set(XML_NS 0)
endif(XML_NS)

if(BUILD_tests)
    enable_testing()
endif(BUILD_tests)

include(${CMAKE_CURRENT_LIST_DIR}/ConfigureChecks.cmake)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/lib)

if (FALSE) # XXX(kitware): Settings handled by VTK.
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -wd4996)
endif(MSVC)
if(WIN32)
    add_definitions(-DWIN32)
    set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Add a suffix, usually d on Windows")
endif(WIN32)
endif ()

set(expat_SRCS
    lib/xmlparse.c
    lib/xmlrole.c
    lib/xmltok.c 
    lib/xmltok_impl.c 
    lib/xmltok_ns.c
)

if(BUILD_shared)
    set(_SHARED SHARED)
    if(WIN32)
        if (FALSE)
        set(expat_SRCS ${expat_SRCS} lib/libexpat.def)
        endif ()
    endif(WIN32)
else(BUILD_shared)
    set(_SHARED STATIC)
    if(WIN32)
        add_definitions(-DXML_STATIC)
    endif(WIN32)
endif(BUILD_shared)

if (FALSE) # XXX(kitware): Use VTK's build macros.
add_library(expat ${_SHARED} ${expat_SRCS})

set(LIBCURRENT 7)   # sync
set(LIBREVISION 2)  # with
set(LIBAGE 6)       # configure.ac!
math(EXPR LIBCURRENT_MINUS_AGE "${LIBCURRENT} - ${LIBAGE}")

if(NOT WIN32)
    set_property(TARGET expat PROPERTY VERSION ${LIBCURRENT_MINUS_AGE}.${LIBAGE}.${LIBREVISION})
    set_property(TARGET expat PROPERTY SOVERSION ${LIBCURRENT_MINUS_AGE})
    set_property(TARGET expat PROPERTY NO_SONAME ${NO_SONAME})
endif(NOT WIN32)

install(TARGETS expat RUNTIME DESTINATION bin
                      LIBRARY DESTINATION lib
                      ARCHIVE DESTINATION lib)
else ()
vtk_add_library(vtkexpat ${expat_SRCS})

set(expat_headers
    lib/expat.h
    lib/expat_external.h
    lib/vtk_expat_mangle.h)

foreach (expat_header IN LISTS expat_headers)
    get_filename_component(expat_header_name "${expat_header}" NAME)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${expat_header}"
        "${CMAKE_CURRENT_BINARY_DIR}/${expat_header_name}"
        COPYONLY)
endforeach ()

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
    install(
        FILES       ${expat_headers}
        DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkexpat"
        COMPONENT   Development)
endif ()
endif ()

if (FALSE) # XXX(kitware): Do not install a .pc file.
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}/bin")
set(libdir "\${prefix}/lib")
set(includedir "\${prefix}/include")
configure_file(expat.pc.in ${CMAKE_CURRENT_BINARY_DIR}/expat.pc)

install(FILES lib/expat.h lib/expat_external.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/expat.pc DESTINATION lib/pkgconfig)
endif ()


if (FALSE) # XXX(kitware): No xmlwf support in VTK.
add_custom_command(TARGET expat PRE_BUILD COMMAND $(MAKE) -C doc xmlwf.1)
endif ()

if(BUILD_tools AND NOT WINCE)
    set(xmlwf_SRCS
        xmlwf/xmlwf.c
        xmlwf/xmlfile.c
        xmlwf/codepage.c
        xmlwf/readfilemap.c
    )

    add_executable(xmlwf ${xmlwf_SRCS})
    set_property(TARGET xmlwf PROPERTY RUNTIME_OUTPUT_DIRECTORY xmlwf)
    target_link_libraries(xmlwf expat)
    install(TARGETS xmlwf DESTINATION bin)
    install(FILES doc/xmlwf.1 DESTINATION share/man/man1)
endif(BUILD_tools AND NOT WINCE)

if(BUILD_examples)
    add_executable(elements examples/elements.c)
    set_property(TARGET elements PROPERTY RUNTIME_OUTPUT_DIRECTORY examples)
    target_link_libraries(elements expat)

    add_executable(outline examples/outline.c)
    set_property(TARGET outline PROPERTY RUNTIME_OUTPUT_DIRECTORY examples)
    target_link_libraries(outline expat)
endif(BUILD_examples)

if(BUILD_tests)
    ## these are unittests that can be run on any platform
    add_executable(runtests tests/runtests.c tests/chardata.c tests/minicheck.c)
    set_property(TARGET runtests PROPERTY RUNTIME_OUTPUT_DIRECTORY tests)
    target_link_libraries(runtests expat)
    add_test(runtests tests/runtests)

    add_executable(runtestspp tests/runtestspp.cpp tests/chardata.c tests/minicheck.c)
    set_property(TARGET runtestspp PROPERTY RUNTIME_OUTPUT_DIRECTORY tests)
    target_link_libraries(runtestspp expat)
    add_test(runtestspp tests/runtestspp)
endif(BUILD_tests)
