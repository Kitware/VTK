#-----------------------------------------------------------------------------
# Build a CPack installer if CPack is available and this is a build of just
# VTK (as opposed to a build of VTK included in some other project...)

IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
  IF("${VTK_BINARY_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

    # For now, only build the CPack installer if vtk(.exe) will be available for
    # installation:
    #
    IF(VTK_WRAP_TCL)
      SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "VTK - The Visualization Toolkit")
      SET(CPACK_PACKAGE_VENDOR "Kitware, Inc.")
      SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
      SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
      SET(CPACK_PACKAGE_VERSION_MAJOR "${VTK_MAJOR_VERSION}")
      SET(CPACK_PACKAGE_VERSION_MINOR "${VTK_MINOR_VERSION}")
      SET(CPACK_PACKAGE_VERSION_PATCH "${VTK_BUILD_VERSION}")
      SET(CPACK_PACKAGE_INSTALL_DIRECTORY "VTK ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
      SET(CPACK_SOURCE_PACKAGE_FILE_NAME "vtk-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
      SET(CPACK_PACKAGE_EXECUTABLES
        "vtk" "VTK"
        )

      IF(WIN32)
        STRING(REGEX REPLACE "/" "\\\\\\\\" CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Release/VTKInstall.bmp")

        SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\vtk.exe")
        SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
        SET(CPACK_NSIS_HELP_LINK "http://www.vtk.org")
        SET(CPACK_NSIS_URL_INFO_ABOUT "http://www.kitware.com")
        SET(CPACK_NSIS_CONTACT "kitware@kitware.com")
      ENDIF(WIN32)

      INCLUDE(CPack)
    ENDIF(VTK_WRAP_TCL)

  ENDIF("${VTK_BINARY_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
