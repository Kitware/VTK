# Copyright (c) 2003,2006 Sandia National Laboratories <cubit@sandia.gov>
project(verdict)

cmake_minimum_required( VERSION 2.4.5 )
if ( COMMAND CMAKE_POLICY )
  cmake_policy( SET CMP0003 NEW )
endif( COMMAND CMAKE_POLICY )

set( verdict_MAJOR_VERSION "1")
set( verdict_MINOR_VERSION "2")
set( verdict_BUILD_VERSION "0")
set( verdict_VERSION_FLAT "${verdict_MAJOR_VERSION}${verdict_MINOR_VERSION}${verdict_BUILD_VERSION}" )
set( verdict_VERSION "${verdict_MAJOR_VERSION}.${verdict_MINOR_VERSION}.${verdict_BUILD_VERSION}" )

option(BUILD_SHARED_LIBS "Build Verdict with shared libraries." OFF)
option(VERDICT_BUILD_DOC "Build the 2007 Verdict User Manual" OFF)
option(VERDICT_MANGLE "Mangle verdict names for inclusion in a larger library?" OFF)
if ( VERDICT_MANGLE )
  set( VERDICT_MANGLE_PREFIX "" CACHE STRING "A string to prepend to all verdict function names and classes." )
  mark_as_advanced( VERDICT_MANGLE_PREFIX )
  configure_file(
    ${verdict_SOURCE_DIR}/verdict_mangle.h.in
    ${verdict_BINARY_DIR}/verdict_mangle.h
  )
endif ( VERDICT_MANGLE )
mark_as_advanced( VERDICT_MANGLE )

option( VERDICT_ENABLE_TESTING "Should tests of the VERDICT library be built?" OFF )

include_directories(
  ${verdict_BINARY_DIR}
  ${verdict_SOURCE_DIR}
)

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

configure_file(
  ${verdict_SOURCE_DIR}/verdict.h.in
  ${verdict_BINARY_DIR}/verdict.h
  @ONLY
)

if ( VERDICT_MANGLE )
  set( verdict_LIBRARY "${VERDICT_MANGLE_PREFIX}verdict" )
else ( VERDICT_MANGLE )
  set( verdict_LIBRARY "verdict" )
endif ( VERDICT_MANGLE )

# Setting the VERSION and SOVERSION of a library will include
# version information either in the library, or in the library
# name (depending on the platform). You may choose to exclude
# this information.
if ( NOT VERDICT_NO_LIBRARY_VERSION )
  set( VERDICT_LIBRARY_PROPERTIES
    ${VERDICT_LIBRARY_PROPERTIES}
    VERSION "${verdict_VERSION}"
    SOVERSION "${verdict_MAJOR_VERSION}.${verdict_MINOR_VERSION}"
  )
endif ( NOT VERDICT_NO_LIBRARY_VERSION )

vtk_add_library( ${verdict_LIBRARY} ${verdict_SRCS} )
set_target_properties( ${verdict_LIBRARY} PROPERTIES DEFINE_SYMBOL verdict_EXPORTS)

# Apply user-defined properties to the library targets.
if ( VERDICT_LIBRARY_PROPERTIES )
  set_target_properties( ${verdict_LIBRARY}
    PROPERTIES ${VERDICT_LIBRARY_PROPERTIES}
  )
endif ( VERDICT_LIBRARY_PROPERTIES )

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

if (VTK_INSTALL_BIN_DIR_CM24)
  set (verdict_INSTALL_BIN_DIR ${VTK_INSTALL_BIN_DIR_CM24})
endif(VTK_INSTALL_BIN_DIR_CM24)

if(VTK_INSTALL_LIB_DIR_CM24)
  set (verdict_INSTALL_LIB_DIR ${VTK_INSTALL_LIB_DIR_CM24})
endif(VTK_INSTALL_LIB_DIR_CM24)

if(VTK_INSTALL_INCLUDE_DIR)
  set (verdict_INSTALL_INCLUDE_SUBDIR ${VTK_INSTALL_INCLUDE_DIR})
endif(VTK_INSTALL_INCLUDE_DIR)

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
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    README
    Verdict.htm
    Verdict.doc
    verdict_test.cpp
    DESTINATION ${verdict_INSTALL_DOC_DIR}/verdict/${verdict_VERSION}/
    COMPONENT Development
  )

  install(
    FILES ${verdict_BINARY_DIR}/verdict.h
    DESTINATION ${verdict_INSTALL_INCLUDE_DIR}
    COMPONENT Development
  )

  if ( VERDICT_MANGLE )
    install(
      FILES ${verdict_BINARY_DIR}/verdict_mangle.h
      DESTINATION ${verdict_INSTALL_INCLUDE_DIR}
      COMPONENT Development
    )
  endif ( VERDICT_MANGLE )

ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

install(TARGETS ${verdict_LIBRARY}
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
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
