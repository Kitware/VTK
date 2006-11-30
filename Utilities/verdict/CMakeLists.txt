# Copyright (c) 2003,2006 Sandia National Laboratories <cubit@sandia.gov>
project(verdict)

set( verdict_version_major "1" )
set( verdict_version_minor "1" )
set( verdict_version_patch "2" )
set( verdict_libname "verdict${verdict_version_major}${verdict_version_minor}${verdict_version_patch}" )
set( VERDICT_VERSION "${verdict_version_major}${verdict_version_minor}${verdict_version_patch}" )
set( VERDICT_DOTVERSION "${verdict_version_major}.${verdict_version_minor}.${verdict_version_patch}" )

option( VERDICT_USE_FLOAT "Should VERDICT use single-precision (float) or double-precision (double) numbers?" ON )
option( VERDICT_SHARED_LIB "Should the VERDICT library be shared or statically linked?" ON )
option( VERDICT_ENABLE_TESTING "Should tests of the VERDICT library be built?" OFF )

set( BUILD_SHARED_LIBS ${VERDICT_SHARED_LIB} )
mark_as_advanced( BUILD_SHARED_LIBS )
set( EXECUTABLE_OUTPUT_PATH "${verdict_BINARY_DIR}/bin" )
mark_as_advanced( EXECUTABLE_OUTPUT_PATH )
set( LIBRARY_OUTPUT_PATH "${verdict_BINARY_DIR}/bin" )
mark_as_advanced( LIBRARY_OUTPUT_PATH )

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
)

add_library( "${verdict_libname}" ${verdict_SRCS} )

if ( VERDICT_ENABLE_TESTING )
  enable_testing()

  set( verdict_TESTSRCS
    verdict_test.cpp
  )

  add_executable( verdict_test ${verdict_TESTSRCS} )
  target_link_libraries( verdict_test "${verdict_libname}" )

  add_test( verdict_test verdict_test )
  set_tests_properties( verdict_test PROPERTIES FAIL_REGULAR_EXPRESSION "FAILED" )
endif ( VERDICT_ENABLE_TESTING )

#
# Installation stuff
#
INSTALL(FILES 
  README
  LICENSE
  Verdict.htm
  Verdict.doc
  docs/hex.tex
  docs/hex.eps
  docs/tri.tex
  docs/tri.eps
  docs/quad.tex
  docs/quad.eps
  verdict_test.cpp
  DESTINATION ${VTK_INSTALL_DOC_DIR_CM24}/verdict/1.0.1
  COMPONENT Development
)

INSTALL(FILES ${verdict_BINARY_DIR}/verdict.h
  DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
  COMPONENT Development
)

INSTALL(TARGETS "${verdict_libname}"
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries # .exe, .dll
      LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries # .so, mod.dll
      ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development      # .a, .lib
)

#
# Packing stuff
#
if (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
  if (EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")
      set (CMAKE_INSTALL_MFC_LIBRARIES 1)
      include (InstallRequiredSystemLibraries)
  endif (EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")
  set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "VERDICT - Geometric finite element quality metrics library")
  set (CPACK_PACKAGE_VENDOR "Sandia National Laboratories")
  set (CPACK_PACKAGE_INSTALL_DIRECTORY "Verdict ${verdict_version_major}.${verdict_version_minor}")
  set (CPACK_SOURCE_PACKAGE_FILE_NAME "verdict-${verdict_version_major}.${verdict_version_minor}.${verdict_version_patch}")
  set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
  set (CPACK_PACKAGE_VERSION_MAJOR "${verdict_version_major}")
  set (CPACK_PACKAGE_VERSION_MINOR "${verdict_version_minor}")
  set (CPACK_PACKAGE_VERSION_PATCH "${verdict_version_patch}")
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
    set (CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} a geometric finite element quality metrics library")
    set (CPACK_NSIS_HELP_LINK "http:\\\\\\\\cubit.sandia.gov/verdict.html")
    set (CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.sandia.gov")
    set (CPACK_NSIS_CONTACT "cubit@sandia.gov")
    set (CPACK_NSIS_MODIFY_PATH ON)
  else (WIN32 AND NOT UNIX)
    #set (CPACK_STRIP_FILES "bin/ccmake;bin/cmake;bin/cpack;bin/ctest")
    #set (CPACK_PACKAGE_EXECUTABLES "")
  endif (WIN32 AND NOT UNIX)
  include(CPack)
endif (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
