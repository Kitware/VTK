# Generate the VTKConfig.cmake file in the build tree.  Also configure
# one for installation.  The file tells external projects how to use
# VTK.

#-----------------------------------------------------------------------------
# Settings shared between the build tres and install tree.

IF(VTK_USE_MPI)
  SET(VTK_MPIRUN_EXE_CONFIG ${VTK_MPIRUN_EXE})
  SET(VTK_MPI_MAX_NUMPROCS_CONFIG ${VTK_MPI_MAX_NUMPROCS})
  SET(VTK_MPI_POSTFLAGS_CONFIG ${VTK_MPI_POSTFLAGS})
  SET(VTK_MPI_PREFLAGS_CONFIG ${VTK_MPI_PREFLAGS})
ELSE(VTK_USE_MPI)
  SET(VTK_MPIRUN_EXE_CONFIG "")
  SET(VTK_MPI_MAX_NUMPROCS_CONFIG "")
  SET(VTK_MPI_POSTFLAGS_CONFIG "")
  SET(VTK_MPI_PREFLAGS_CONFIG "")
ENDIF(VTK_USE_MPI)

#-----------------------------------------------------------------------------
# Settings specific to the build tree.

# The "use" file.
SET(VTK_USE_FILE ${VTK_BINARY_DIR}/UseVTK.cmake)

# The library dependencies file.
SET(VTK_LIBRARY_DEPENDS_FILE ${VTK_BINARY_DIR}/VTKLibraryDepends.cmake)

# The build settings file.
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
  SET(VTK_BUILD_SETTINGS_FILE ${VTK_BINARY_DIR}/VTKBuildSettings.cmake)
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)

# Library directory.
SET(VTK_LIBRARY_DIRS_CONFIG ${LIBRARY_OUTPUT_PATH})

# Determine the include directories needed.
SET(VTK_INCLUDE_DIRS_CONFIG
  ${VTK_INCLUDE_DIRS_BUILD_TREE}
  ${VTK_INCLUDE_DIRS_SOURCE_TREE}
  ${VTK_INCLUDE_DIRS_SYSTEM}
)

# Executable locations.
SET(VTK_TCL_HOME_CONFIG "")
SET(VTK_JAVA_JAR_CONFIG "")
SET(VTK_PARSE_JAVA_EXE_CONFIG "")
SET(VTK_WRAP_JAVA_EXE_CONFIG "")
SET(VTK_WRAP_PYTHON_EXE_CONFIG "")
SET(VTK_WRAP_TCL_EXE_CONFIG "")
IF(VTK_WRAP_TCL)
  SET(VTK_WRAP_TCL_EXE_CONFIG ${VTK_WRAP_TCL_EXE})
  SET(VTK_TCL_HOME_CONFIG ${VTK_TCL_HOME})
ENDIF(VTK_WRAP_TCL)
IF(VTK_WRAP_PYTHON)
  SET(VTK_WRAP_PYTHON_EXE_CONFIG ${VTK_WRAP_PYTHON_EXE})
ENDIF(VTK_WRAP_PYTHON)
IF(VTK_WRAP_JAVA)
  SET(VTK_PARSE_JAVA_EXE_CONFIG ${VTK_PARSE_JAVA_EXE})
  SET(VTK_WRAP_JAVA_EXE_CONFIG ${VTK_WRAP_JAVA_EXE})
  SET(VTK_JAVA_JAR_CONFIG ${LIBRARY_OUTPUT_PATH}/vtk.jar)
ENDIF(VTK_WRAP_JAVA)

# VTK style script locations.
SET(VTK_DOXYGEN_HOME_CONFIG ${VTK_SOURCE_DIR}/Utilities/Doxygen)
SET(VTK_HEADER_TESTING_PY_CONFIG ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py)
SET(VTK_FIND_STRING_TCL_CONFIG ${VTK_SOURCE_DIR}/Common/Testing/Tcl/FindString.tcl)
SET(VTK_PRINT_SELF_CHECK_TCL_CONFIG ${VTK_SOURCE_DIR}/Common/Testing/Tcl/PrintSelfCheck.tcl)
SET(VTK_RT_IMAGE_TEST_TCL_CONFIG ${VTK_SOURCE_DIR}/Common/Testing/Tcl/rtImageTest.tcl)

IF(VTK_USE_PARALLEL)
  SET(VTK_PRT_IMAGE_TEST_TCL_CONFIG ${VTK_SOURCE_DIR}/Common/Testing/Tcl/prtImageTest.tcl)
ELSE(VTK_USE_PARALLEL)
  SET(VTK_PRT_IMAGE_TEST_TCL_CONFIG "")
ENDIF(VTK_USE_PARALLEL)

# Location of tk internal headers provided by VTK.
IF(VTK_RENDERING_NEED_TK_INTERNAL)
  SET(VTK_TK_INTERNAL_DIR_CONFIG ${TK_INTERNAL_PATH})
ELSE(VTK_RENDERING_NEED_TK_INTERNAL)
  SET(VTK_TK_INTERNAL_DIR_CONFIG "")
ENDIF(VTK_RENDERING_NEED_TK_INTERNAL)

# CMake extension module directory.
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
  SET(VTK_CMAKE_EXTENSIONS_DIR_CONFIG ${VTK_SOURCE_DIR}/CMake)
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)

#-----------------------------------------------------------------------------
# Configure VTKConfig.cmake for the build tree.
CONFIGURE_FILE(${VTK_SOURCE_DIR}/VTKConfig.cmake.in
               ${VTK_BINARY_DIR}/VTKConfig.cmake @ONLY IMMEDIATE)

# Hack to give source tree access for a build tree configuration.
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
  STRING(ASCII 35 VTK_STRING_POUND)
  STRING(ASCII 64 VTK_STRING_AT)
  WRITE_FILE(${VTK_BINARY_DIR}/VTKConfig.cmake
    "\n"
    "${VTK_STRING_POUND} For backward compatability.  DO NOT USE.\n"
    "SET(VTK_SOURCE_DIR \"${VTK_SOURCE_DIR}\")\n"
    "IF(NOT TCL_LIBRARY)\n"
    "  SET(TCL_LIBRARY \"${TCL_LIBRARY}\" CACHE FILEPATH \"Location of Tcl library imported from VTK.  This may mean your project is depending on VTK to get this setting.  Consider using FindTCL.cmake.\")\n"
    "ENDIF(NOT TCL_LIBRARY)\n"
    "IF(NOT TK_LIBRARY)\n"
    "  SET(TK_LIBRARY \"${TK_LIBRARY}\" CACHE FILEPATH \"Location of Tk library imported from VTK.  This may mean your project is depending on VTK to get this setting.  Consider using FindTCL.cmake.\")\n"
    "ENDIF(NOT TK_LIBRARY)\n"
    "MARK_AS_ADVANCED(TCL_LIBRARY TK_LIBRARY)\n"
    APPEND
  )
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)

#-----------------------------------------------------------------------------
# Settings specific to the install tree.

# The "use" file.
SET(VTK_USE_FILE ${CMAKE_INSTALL_PREFIX}/lib/vtk/UseVTK.cmake)

# The library dependencies file.
SET(VTK_LIBRARY_DEPENDS_FILE ${CMAKE_INSTALL_PREFIX}/lib/vtk/VTKLibraryDepends.cmake)

# The build settings file.
IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
  SET(VTK_BUILD_SETTINGS_FILE ${CMAKE_INSTALL_PREFIX}/lib/vtk/VTKBuildSettings.cmake)
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)

# Include directories.
SET(VTK_INCLUDE_DIRS_CONFIG
  ${CMAKE_INSTALL_PREFIX}/include/vtk
  ${VTK_INCLUDE_DIRS_SYSTEM}
)

# Link directories.
SET(VTK_LIBRARY_DIRS_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk)

# Executable locations.
SET(VTK_TCL_HOME_CONFIG "")
SET(VTK_JAVA_JAR_CONFIG "")
SET(VTK_PARSE_JAVA_EXE_CONFIG "")
SET(VTK_WRAP_JAVA_EXE_CONFIG "")
SET(VTK_WRAP_PYTHON_EXE_CONFIG "")
SET(VTK_WRAP_TCL_EXE_CONFIG "")
SET(VTK_DOXYGEN_HOME_CONFIG "")
IF(VTK_WRAP_TCL)
  SET(VTK_WRAP_TCL_EXE_CONFIG ${CMAKE_INSTALL_PREFIX}/bin/vtkWrapTcl)
  SET(VTK_TCL_HOME_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/tcl)
ENDIF(VTK_WRAP_TCL)
IF(VTK_WRAP_PYTHON)
  SET(VTK_WRAP_PYTHON_EXE_CONFIG ${CMAKE_INSTALL_PREFIX}/bin/vtkWrapPython)
ENDIF(VTK_WRAP_PYTHON)
IF(VTK_WRAP_JAVA)
  SET(VTK_PARSE_JAVA_EXE_CONFIG ${CMAKE_INSTALL_PREFIX}/bin/vtkParseJava)
  SET(VTK_WRAP_JAVA_EXE_CONFIG ${CMAKE_INSTALL_PREFIX}/bin/vtkWrapJava)
  SET(VTK_JAVA_JAR_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/java/vtk.jar)
ENDIF(VTK_WRAP_JAVA)

# VTK style script locations.
SET(VTK_DOXYGEN_HOME_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/doxygen)
SET(VTK_HEADER_TESTING_PY_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/testing/HeaderTesting.py)
SET(VTK_FIND_STRING_TCL_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/testing/FindString.tcl)
SET(VTK_PRINT_SELF_CHECK_TCL_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/testing/PrintSelfCheck.tcl)
SET(VTK_RT_IMAGE_TEST_TCL_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/testing/rtImageTest.tcl)

IF(VTK_USE_PARALLEL)
  SET(VTK_PRT_IMAGE_TEST_TCL_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/testing/prtImageTest.tcl)
ELSE(VTK_USE_PARALLEL)
  SET(VTK_PRT_IMAGE_TEST_TCL_CONFIG "")
ENDIF(VTK_USE_PARALLEL)

# Location of tk internal headers provided by VTK.
SET(VTK_TK_INTERNAL_DIR_CONFIG "")
IF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_INTERNAL_PATH)
  IF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk82")
    SET(VTK_TK_INTERNAL_DIR_CONFIG ${CMAKE_INSTALL_PREFIX}/include/vtk/tkInternals/tk82)
  ENDIF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk82")
  IF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk83")
    SET(VTK_TK_INTERNAL_DIR_CONFIG ${CMAKE_INSTALL_PREFIX}/include/vtk/tkInternals/tk83)
  ENDIF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk83")
  IF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk84")
    SET(VTK_TK_INTERNAL_DIR_CONFIG ${CMAKE_INSTALL_PREFIX}/include/vtk/tkInternals/tk84)
  ENDIF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk84")
  IF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk84OSX")
    SET(VTK_TK_INTERNAL_DIR_CONFIG ${CMAKE_INSTALL_PREFIX}/include/vtk/tkInternals/tk84OSX)
  ENDIF("${TK_INTERNAL_PATH}" MATCHES "Rendering/tkInternals/tk84OSX")
  IF(NOT VTK_TK_INTERNAL_DIR_CONFIG)
    SET(VTK_TK_INTERNAL_DIR_CONFIG ${TK_INTERNAL_PATH})
  ENDIF(NOT VTK_TK_INTERNAL_DIR_CONFIG)
ENDIF(VTK_RENDERING_NEED_TK_INTERNAL AND TK_INTERNAL_PATH)

SET(VTK_TK_INTERNAL_ROOT_CONFIG ${CMAKE_INSTALL_PREFIX}/include/vtk)

# CMake extension module directory.
SET(VTK_CMAKE_EXTENSIONS_DIR_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/vtk/CMake)

#-----------------------------------------------------------------------------
# Configure VTKConfig.cmake for the install tree.
CONFIGURE_FILE(${VTK_SOURCE_DIR}/VTKConfig.cmake.in
               ${VTK_BINARY_DIR}/Utilities/VTKConfig.cmake @ONLY IMMEDIATE)

#-----------------------------------------------------------------------------
# Configure VTKLibraryDependencies.cmake for both trees.

IF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
  SET(VTK_LIBS
    vtkCommon vtkCommonJava vtkCommonPython vtkCommonTCL
    vtkFiltering vtkFilteringJava vtkFilteringPython vtkFilteringTCL
    vtkGraphics vtkGraphicsJava vtkGraphicsPython vtkGraphicsTCL
    vtkHybrid vtkHybridJava vtkHybridPython vtkHybridTCL
    vtkIO vtkIOJava vtkIOPython vtkIOTCL
    vtkImaging vtkImagingJava vtkImagingPython vtkImagingTCL
    vtkParallel vtkParallelJava vtkParallelPython vtkParallelTCL
    vtkPatented vtkPatentedJava vtkPatentedPython vtkPatentedTCL
    vtkRendering vtkRenderingJava vtkRenderingPython vtkRenderingTCL
    vtkRenderingPythonTkWidgets
    vtkexpat vtkfreetype vtkftgl vtkjpeg vtkpng vtktiff vtkzlib
    )

  # Write an input file that will be configured.
  STRING(ASCII 35 VTK_STRING_POUND)
  STRING(ASCII 64 VTK_STRING_AT)
  WRITE_FILE(${VTK_BINARY_DIR}/VTKLibraryDepends.cmake.in
             "${VTK_STRING_POUND} VTK Library Dependencies (for external projects)")
  FOREACH(lib ${VTK_LIBS})
    WRITE_FILE(${VTK_BINARY_DIR}/VTKLibraryDepends.cmake.in
     "SET(${lib}_LIB_DEPENDS \"${VTK_STRING_AT}${lib}_LIB_DEPENDS${VTK_STRING_AT}\")"
      APPEND
    )
  ENDFOREACH(lib)

  # Configure the file during the final pass so that the latest settings
  # for the *_LIB_DEPENDS cache entries will be available.
  CONFIGURE_FILE(${VTK_BINARY_DIR}/VTKLibraryDepends.cmake.in
                 ${VTK_BINARY_DIR}/VTKLibraryDepends.cmake @ONLY)
ELSE(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
  CONFIGURE_FILE(${VTK_SOURCE_DIR}/VTKLibraryDepends.cmake14.in
                 ${VTK_BINARY_DIR}/VTKLibraryDepends.cmake @ONLY)
ENDIF(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
