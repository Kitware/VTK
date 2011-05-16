#-----------------------------------------------------------------------------
# Build a CPack installer if CPack is available and this is a build of just
# VTK (as opposed to a build of VTK included in some other project...)

IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
  IF("${VTK_BINARY_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

    # For now, only build the CPack installer if vtk(.exe) or vtkpython will be available for
    # installation:
    #
    IF(VTK_WRAP_TCL OR VTK_WRAP_PYTHON)
      # Disable component based installation.
      set(CPACK_MONOLITHIC_INSTALL ON)
      
      SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "VTK - The Visualization Toolkit")
      SET(CPACK_PACKAGE_VENDOR "Kitware, Inc.")
      SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
      SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
      
      IF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")
        SET (CMAKE_SYSTEM_PROCESSOR "x86")
      ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")

      IF(NOT DEFINED CPACK_SYSTEM_NAME)
        SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
      ENDIF(NOT DEFINED CPACK_SYSTEM_NAME)

      IF(${CPACK_SYSTEM_NAME} MATCHES Windows)
        IF(CMAKE_CL_64)
          SET(CPACK_SYSTEM_NAME win64-${CMAKE_SYSTEM_PROCESSOR})
        ELSE(CMAKE_CL_64)
          SET(CPACK_SYSTEM_NAME win32-${CMAKE_SYSTEM_PROCESSOR})
        ENDIF(CMAKE_CL_64)
      ENDIF(${CPACK_SYSTEM_NAME} MATCHES Windows)
      
      IF(${CPACK_SYSTEM_NAME} MATCHES Darwin AND CMAKE_OSX_ARCHITECTURES)
        LIST(LENGTH CMAKE_OSX_ARCHITECTURES _length)
        IF(_length GREATER 1)
          SET(CPACK_SYSTEM_NAME Darwin-Universal)
        ELSE(_length GREATER 1)
          SET(CPACK_SYSTEM_NAME Darwin-${CMAKE_OSX_ARCHITECTURES})
        ENDIF(_length GREATER 1)
      ENDIF(${CPACK_SYSTEM_NAME} MATCHES Darwin AND CMAKE_OSX_ARCHITECTURES)

      SET(CPACK_PACKAGE_VERSION_MAJOR "${VTK_MAJOR_VERSION}")
      SET(CPACK_PACKAGE_VERSION_MINOR "${VTK_MINOR_VERSION}")
      SET(CPACK_PACKAGE_VERSION_PATCH "${VTK_BUILD_VERSION}")
      SET(CPACK_PACKAGE_INSTALL_DIRECTORY "VTK ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
      SET(CPACK_SOURCE_PACKAGE_FILE_NAME "vtk-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
      SET(CPACK_PACKAGE_EXECUTABLES
        "vtk" "VTK"
        "vtkpython" "VTKPython"
        )

      # Set VTK Components to be installed
      SET (CPACK_INSTALL_CMAKE_PROJECTS
        "${VTK_BINARY_DIR}" "VTK Runtime Executables" "RuntimeExecutables" "/"
        "${VTK_BINARY_DIR}" "VTK Runtime Libs" "RuntimeLibraries" "/"
      )

      # Append in CPACK rule for the Development Component
      IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
        LIST(APPEND CPACK_INSTALL_CMAKE_PROJECTS
        "${VTK_BINARY_DIR}" "VTK Development Headers, Libs and Tools" "Development" "/"
        )
      ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

      IF(WIN32)
        STRING(REGEX REPLACE "/" "\\\\\\\\" CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Release/VTKInstall.bmp")
        SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\vtk.exe")
        set(CPACK_NSIS_MUI_ICON "${VTK_SOURCE_DIR}\\\\vtkLogo.ico")
        set(CPACK_NSIS_MUI_UNIICON "${VTK_SOURCE_DIR}\\\\vtkLogo.ico")
        SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY}")
        SET(CPACK_NSIS_HELP_LINK "http://www.vtk.org")
        SET(CPACK_NSIS_URL_INFO_ABOUT "http://www.kitware.com")
        SET(CPACK_NSIS_CONTACT "kitware@kitware.com")
      ENDIF(WIN32)

      INCLUDE(CPack)
    ENDIF(VTK_WRAP_TCL OR VTK_WRAP_PYTHON)

  ENDIF("${VTK_BINARY_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
