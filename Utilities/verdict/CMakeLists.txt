# Copyright (c) 2003,2006 Sandia National Laboratories <cubit@sandia.gov>
project(verdict)

option(BUILD_SHARED_LIBS "Build Verdict with shared libraries." OFF)
option(VERDICT_BUILD_DOC "Build the 2007 Verdict User Manual" OFF)

set( verdict_MAJOR_VERSION "1")
set( verdict_MINOR_VERSION "2")
set( verdict_BUILD_VERSION "0")

set( verdict_VERSION_FLAT "${verdict_MAJOR_VERSION}${verdict_MINOR_VERSION}${verdict_BUILD_VERSION}" )
set( verdict_VERSION "${verdict_MAJOR_VERSION}.${verdict_MINOR_VERSION}.${verdict_BUILD_VERSION}" )
option( VERDICT_ENABLE_TESTING "Should tests of the VERDICT library be built?" OFF )


include_directories(${verdict_BINARY_DIR})
include_directories(${verdict_SOURCE_DIR})

set( verdict_SRCS
  V_EdgeMetric.cpp
  V_GaussIntegration.cpp
  V_HexMetric.cpp
  V_KnifeMetric.cpp
  V_PyramidMetric.cpp
  V_QuadMetric.cpp
  V_TetMetric.cpp
  V_TriMetric.cpp
  VerdictVector.cpp
  V_WedgeMetric.cpp
  )

set(verdict_INCLUDE_SUBDIR "include")
configure_file(
  ${verdict_SOURCE_DIR}/verdict.h.in
  ${verdict_BINARY_DIR}/verdict.h
)

if(NOT DEFINED verdict_LIBRARY)
  set(verdict_LIBRARY verdict)
else(NOT DEFINED verdict_LIBRARY)
  # if the name of the library is not 'verdict', then add verdict_EXPORTS 
  # manually so that the usual dllexport/dllimport tricks still work.
  if(WIN32)
    set_source_files_properties(${verdict_SRCS} PROPERTIES
      COMPILE_FLAGS "-Dverdict_EXPORTS")
  endif(WIN32)
endif(NOT DEFINED verdict_LIBRARY)
add_library( ${verdict_LIBRARY} ${verdict_SRCS} )
set_target_properties(${verdict_LIBRARY} PROPERTIES 
  VERSION ${verdict_VERSION}
  SOVERSION ${verdict_MAJOR_VERSION}.${verdict_MINOR_VERSION}
)

if ( VERDICT_ENABLE_TESTING )
  enable_testing()

  set( verdict_TESTSRCS
    verdict_test.cpp
  )

  add_executable( verdict_test ${verdict_TESTSRCS} )
  target_link_libraries( verdict_test ${verdict_LIBRARY} )

  add_test( verdict_test verdict_test )
  set_tests_properties( verdict_test PROPERTIES FAIL_REGULAR_EXPRESSION "FAILED" )
endif ( VERDICT_ENABLE_TESTING )

if ( NOT verdict_INSTALL_DOC_DIR )
  set (verdict_INSTALL_DOC_DIR doc)
endif( NOT verdict_INSTALL_DOC_DIR )
if ( NOT verdict_INSTALL_INCLUDE_DIR)
  set (verdict_INSTALL_INCLUDE_DIR include)
endif ( NOT verdict_INSTALL_INCLUDE_DIR)
if ( NOT verdict_INSTALL_INCLUDE_SUBDIR)
  set (verdict_INSTALL_INCLUDE_SUBDIR verdict)
endif ( NOT verdict_INSTALL_INCLUDE_SUBDIR)
if ( NOT verdict_INSTALL_BIN_DIR)
  set (verdict_INSTALL_BIN_DIR bin)
endif ( NOT verdict_INSTALL_BIN_DIR)
if ( NOT verdict_INSTALL_LIB_DIR)
  set (verdict_INSTALL_LIB_DIR lib)
endif ( NOT verdict_INSTALL_LIB_DIR)

if ( VERDICT_BUILD_DOC )
  add_subdirectory( docs/VerdictUserManual2007 )
endif ( VERDICT_BUILD_DOC )

#
# Installation stuff
#
install(FILES 
  README
  Verdict.htm
  Verdict.doc
  verdict_test.cpp
  DESTINATION ${verdict_INSTALL_DOC_DIR}/verdict/${verdict_VERSION}/
  COMPONENT Development
)

# verdict.h for the installed tree, which in turns include 
# "${verdict_INSTALL_INCLUDE_SUBDIR}/verdict.h"
# verdict.h for the build tree includes "include/verdict.h"

set(verdict_INCLUDE_SUBDIR "${verdict_INSTALL_INCLUDE_SUBDIR}")
configure_file(
  ${verdict_SOURCE_DIR}/verdict.h.in
  ${verdict_BINARY_DIR}/Install/verdict.h
)

install(FILES ${verdict_BINARY_DIR}/Install/verdict.h
  DESTINATION ${verdict_INSTALL_INCLUDE_DIR}
  COMPONENT Development
)

install(FILES ${verdict_SOURCE_DIR}/include/verdict.h
  DESTINATION ${verdict_INSTALL_INCLUDE_DIR}/${verdict_INSTALL_INCLUDE_SUBDIR}
  COMPONENT Development
)

install(TARGETS ${verdict_LIBRARY}
      RUNTIME DESTINATION ${verdict_INSTALL_BIN_DIR} COMPONENT RuntimeLibraries # .exe, .dll
      LIBRARY DESTINATION ${verdict_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries # .so, mod.dll
      ARCHIVE DESTINATION ${verdict_INSTALL_LIB_DIR} COMPONENT Development      # .a, .lib
)

#
# Packing stuff
#
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  if (EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")
    set (CMAKE_INSTALL_MFC_LIBRARIES 1)
#      include (InstallRequiredSystemLibraries)
  endif (EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")
  set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "VERDICT - a geometric quality functions library")
  set (CPACK_PACKAGE_VENDOR "Sandia National Laboratories")
  set (CPACK_PACKAGE_INSTALL_DIRECTORY "Verdict ${verdict_MAJOR_VERSION}.${verdict_MINOR_VERSION}")
  set (CPACK_SOURCE_PACKAGE_FILE_NAME "verdict-${verdict_VERSION}")
  set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
  set (CPACK_PACKAGE_VERSION_MAJOR "${verdict_MAJOR_VERSION}")
  set (CPACK_PACKAGE_VERSION_MINOR "${verdict_MINOR_VERSION}")
  set (CPACK_PACKAGE_VERSION_PATCH "${verdict_BUILD_VERSION}")
  set (CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
  if (${CMAKE_SYSTEM_NAME} MATCHES Windows)
    if (CMAKE_CL_64)
      set (CPACK_SYSTEM_NAME win64)
    else (CMAKE_CL_64)
      set (CPACK_SYSTEM_NAME win32)
    endif (CMAKE_CL_64)
  endif (${CMAKE_SYSTEM_NAME} MATCHES Windows)

  set (CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
  if (WIN32 AND NOT UNIX)
    # There is a bug in NSI that does not handle full unix paths properly.
    # Make sure there is at least one set of four (4) backslashes.
    set (CPACK_PACKAGE_ICON "${verdict_SOURCE_DIR}/Utilities/Release\\\\VerdictIcon.bmp")
    #set (CPACK_PACKAGE_EXECUTABLES "CMakeSetup" "CMake")
    #set (CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\CMakeSetup.exe")
    set (CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} a geometric quality functions library")
    set (CPACK_NSIS_HELP_LINK "http:\\\\\\\\cubit.sandia.gov/verdict.html")
    set (CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.sandia.gov")
    set (CPACK_NSIS_CONTACT "cubit@sandia.gov")
    set (CPACK_NSIS_MODIFY_PATH ON)
  else (WIN32 AND NOT UNIX)
    #set (CPACK_STRIP_FILES "bin/ccmake;bin/cmake;bin/cpack;bin/ctest")
    #set (CPACK_PACKAGE_EXECUTABLES "")
  endif (WIN32 AND NOT UNIX)
  include(CPack)
endif ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
