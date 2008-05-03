# this file is used directly from VTK/CMakeLists.txt, not from VTK/Wrapping/CMakeLists.txt

IF(VTK_USE_CARBON)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
ENDIF(VTK_USE_CARBON)

IF(UNIX)
  FIND_LIBRARY(PYTHON_UTIL_LIBRARY
    NAMES util
    DOC "Utility library needed for vtkpython"
    )
  MARK_AS_ADVANCED(PYTHON_UTIL_LIBRARY)
ENDIF(UNIX)

IF(UNIX)
  SET(PYTHON_MODULE_PREFIX "lib")
ELSE(UNIX)
  SET(PYTHON_MODULE_PREFIX "")
ENDIF(UNIX)

PYTHON_WRITE_MODULES_HEADER("${CMAKE_CURRENT_BINARY_DIR}/vtkpythonmodules.h")
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

# create the VTK/Python  executable
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h.in
               ${VTK_BINARY_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h)

ADD_EXECUTABLE(vtkpython vtkPythonAppInit.cxx)

SET(VTKPYTHON_LINK_FLAGS)
SET(VTKPYTHON_LINK_LIBS)

IF(CMAKE_SYSTEM_NAME MATCHES "AIX")
  GET_FILENAME_COMPONENT(CMAKE_PYTHON_LIB_PREFIX "${PYTHON_LIBRARY}" PATH)
  FIND_FILE(CMAKE_PYTHON_LIBRARY_EXPORT python.exp "${CMAKE_PYTHON_LIB_PREFIX}")
  IF(CMAKE_PYTHON_LIBRARY_EXPORT)
    SET(VTKPYTHON_LINK_FLAGS "-Wl,-bE:${CMAKE_PYTHON_LIBRARY_EXPORT}")
  ENDIF(CMAKE_PYTHON_LIBRARY_EXPORT)
ENDIF(CMAKE_SYSTEM_NAME MATCHES "AIX")

IF (APPLE)
  IF(VTK_USE_CARBON AND VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(
      TARGET vtkpython POST_BUILD
      COMMAND ${VTK_APPLE_RESOURCE} Carbon.r -o ${VTK_EXECUTABLE_DIR}/vtkpython
      )
  ENDIF(VTK_USE_CARBON AND VTK_APPLE_RESOURCE)

  SET(VTKPYTHON_LINK_FLAGS "-flat_namespace -undefined suppress -u _PyMac_Error")
ENDIF (APPLE)

# Link against all the kit wrappers.
SET(VTKPYTHON_LINK_LIBS 
  ${VTK_PYTHON_LIBRARIES} 
  vtksys
  vtkCommon 
  vtkFiltering
  vtkIO
  vtkGraphics
  vtkImaging )

SET (vtkpython_pyc_depends
    vtkCommonPython
    vtkFilteringPython
    vtkIOPython
    vtkGraphicsPython
    vtkImagingPython
    )

IF(BORLAND)
  SET(KITS Common Filtering Graphics IO Imaging)
  IF (VTK_USE_PARALLEL)
    SET(KITS ${KITS} Parallel)
  ENDIF(VTK_USE_PARALLEL)
  IF (VTK_USE_INFOVIS)
    SET(KITS ${KITS} Infovis)
  ENDIF(VTK_USE_INFOVIS)
  IF (VTK_USE_VIEWS)
    SET(KITS ${KITS} Views)
  ENDIF(VTK_USE_VIEWS)
  IF (VTK_USE_RENDERING)
    SET(KITS ${KITS} Widgets)
    SET(KITS ${KITS} Hybrid)
    SET(KITS ${KITS} VolumeRendering)
    SET(KITS ${KITS} Rendering)
  ENDIF(VTK_USE_RENDERING)
  FOREACH(KIT ${KITS})
    WRITE_FILE(${LIBRARY_OUTPUT_PATH}/vtk${KIT}Python.def
               "EXPORTS\ninitvtk${KIT}Python=_initvtk${KIT}Python\n")
  ENDFOREACH(KIT)
ENDIF(BORLAND)

IF(PYTHON_UTIL_LIBRARY)
  SET(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS}  ${PYTHON_UTIL_LIBRARY})
ENDIF(PYTHON_UTIL_LIBRARY)

IF (VTK_USE_RENDERING)
  SET(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} 
                          vtkRendering 
                          vtkVolumeRendering 
                          vtkHybrid 
                          vtkWidgets )
  SET (vtkpython_pyc_depends
      ${vtkpython_pyc_depends}
      vtkRenderingPython
      vtkVolumeRenderingPython
      vtkHybridPython
      vtkWidgetsPython
      )
ENDIF (VTK_USE_RENDERING)

IF (VTK_USE_PARALLEL)
  SET(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} vtkParallel)
  SET(vtkpython_pyc_depends
      ${vtkpython_pyc_depends}
      vtkParallelPython
      )
ENDIF (VTK_USE_PARALLEL)

IF (VTK_USE_INFOVIS)
  SET(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} vtkInfovis)
  SET(vtkpython_pyc_depends
      ${vtkpython_pyc_depends}
      vtkInfovisPython
      )
ENDIF (VTK_USE_INFOVIS)

IF (VTK_USE_VIEWS)
  SET(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} vtkViews)
  SET(vtkpython_pyc_depends
      ${vtkpython_pyc_depends}
      vtkViewsPython
      )
ENDIF (VTK_USE_VIEWS)

IF(PY_STATIC_MODULES_LIST)
  SET(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} ${PY_STATIC_MODULES_LIST})
ENDIF(PY_STATIC_MODULES_LIST)

# Link to rt to prevent undefined symbol 'fdatasync'
IF(CMAKE_SYSTEM MATCHES "SunOS.*" AND NOT CMAKE_COMPILER_IS_GNUCXX)
  FIND_LIBRARY(VTK_SUNCC_RT_LIBRARY rt)
  IF(VTK_SUNCC_RT_LIBRARY)
    SET(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} ${VTK_SUNCC_RT_LIBRARY})
  ENDIF(VTK_SUNCC_RT_LIBRARY)
ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*" AND NOT CMAKE_COMPILER_IS_GNUCXX)

IF(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES(vtkpython ${VTK_TK_LIBRARIES})
ENDIF(VTK_WRAP_TCL)

TARGET_LINK_LIBRARIES(vtkpython ${VTKPYTHON_LINK_LIBS})

SET_TARGET_PROPERTIES(vtkpython PROPERTIES LINK_FLAGS "${VTKPYTHON_LINK_FLAGS}")

SET(PVTKPYTHON_EXECUTABLE)
# Create the pvtkpython Python wrapper executable with MPI support.
IF (VTK_USE_PARALLEL AND VTK_USE_MPI)
  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")

  SET(PVTKPYTHON_EXECUTABLE pvtkpython)
  ADD_EXECUTABLE(pvtkpython vtkParaPythonAppInit.cxx)
  TARGET_LINK_LIBRARIES(pvtkpython ${VTKPYTHON_LINK_LIBS})
  SET_TARGET_PROPERTIES(pvtkpython PROPERTIES LINK_FLAGS "${VTKPYTHON_LINK_FLAGS}")

  IF(VTK_USE_CARBON AND VTK_APPLE_RESOURCE)
      ADD_CUSTOM_COMMAND( TARGET pvtkpython POST_BUILD
                          COMMAND ${VTK_APPLE_RESOURCE} Carbon.r -o ${VTK_EXECUTABLE_DIR}/pvtkpython )
  ENDIF(VTK_USE_CARBON AND VTK_APPLE_RESOURCE)
ENDIF (VTK_USE_PARALLEL AND VTK_USE_MPI)

IF(NOT VTK_INSTALL_NO_RUNTIME)
  INSTALL(TARGETS vtkpython ${PVTKPYTHON_EXECUTABLE}
    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeExecutables # .exe, .dll
    LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries # .so, mod.dll
    ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development      # .a, .lib
    )
ENDIF(NOT VTK_INSTALL_NO_RUNTIME)

# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to VTK_BINARY_DIR if it is different from VTK_SOURCE_DIR.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***

IF(PYTHON_EXECUTABLE)
# Make the necessary directories.
    FILE(MAKE_DIRECTORY  ${VTK_BINARY_DIR}/Wrapping/Python/vtk/gtk
                         ${VTK_BINARY_DIR}/Wrapping/Python/vtk/qt
                         ${VTK_BINARY_DIR}/Wrapping/Python/vtk/qt4
                         ${VTK_BINARY_DIR}/Wrapping/Python/vtk/tk
                         ${VTK_BINARY_DIR}/Wrapping/Python/vtk/util
                         ${VTK_BINARY_DIR}/Wrapping/Python/vtk/wx
                         ${VTK_BINARY_DIR}/Wrapping/Python/vtk/test )

# Now create a list of Python files.

# Wrapping/Python/vtk/*.py
  SET(VTK_PYTHON_FILES 
    vtk/__helper
    vtk/__init__
    vtk/common
    vtk/filtering
    vtk/genericfiltering
    vtk/graphics
    vtk/hybrid
    vtk/imaging
    vtk/io
    vtk/infovis
    vtk/parallel
    vtk/rendering
    vtk/required
    vtk/views
    vtk/volumerendering
    vtk/widgets
    )

# vtk.util package
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/util/__init__
    vtk/util/vtkMethodParser
    vtk/util/misc
    vtk/util/vtkConstants
    vtk/util/vtkImageExportToArray
    vtk/util/vtkImageImportFromArray
    vtk/util/colors
    vtk/util/numpy_support
    )

# vtk.test package
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/test/BlackBox
    vtk/test/Testing
    vtk/test/__init__
    )

# Tk related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/tk/__init__
    vtk/tk/vtkLoadPythonTkWidgets
    vtk/tk/vtkTkImageViewerWidget
    vtk/tk/vtkTkRenderWidget
    vtk/tk/vtkTkRenderWindowInteractor
    vtk/tk/vtkTkPhotoImage
    )

# PyQt v3 related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/qt/QVTKRenderWidget
    vtk/qt/__init__
    vtk/qt/QVTKRenderWindowInteractor
    )

# PyQt v4 related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/qt4/__init__
    vtk/qt4/QVTKRenderWindowInteractor
    )

# wxPython related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/wx/__init__
    vtk/wx/wxVTKRenderWindow
    vtk/wx/wxVTKRenderWindowInteractor
    )

# pyGTK related files
  SET(VTK_PYTHON_FILES 
    ${VTK_PYTHON_FILES}
    vtk/gtk/GtkVTKRenderWindow
    vtk/gtk/__init__
    vtk/gtk/GtkVTKRenderWindowInteractor
    )

# Done listing of files.

# Now copy these files if necessary.
  SET(VTK_PYTHON_SOURCE_FILES)
  SET(VTK_PYTHON_OUTPUT_FILES)
  FOREACH(file ${VTK_PYTHON_FILES})
    SET(src "${VTK_BINARY_DIR}/Wrapping/Python/${file}.py")
    SET(VTK_PYTHON_SOURCE_FILES ${VTK_PYTHON_SOURCE_FILES} ${src})
  ENDFOREACH(file)

  IF (NOT "${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    FOREACH(file ${VTK_PYTHON_FILES})
      SET(src "${VTK_SOURCE_DIR}/Wrapping/Python/${file}.py")
      SET(tgt "${VTK_BINARY_DIR}/Wrapping/Python/${file}.py")
      ADD_CUSTOM_COMMAND(DEPENDS ${src}
                         COMMAND ${CMAKE_COMMAND} -E copy ${src} ${tgt}
                         OUTPUT ${tgt}
                         COMMENT "source copy")
    ENDFOREACH(file)
  ENDIF (NOT "${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")

  # handle the different configuration types (each will have identical files)
  SET(VTK_PYTHON_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
  IF(VTK_PYTHON_CONFIGURATION_TYPES)
    FOREACH(config ${VTK_PYTHON_CONFIGURATION_TYPES})
      FOREACH(file ${VTK_PYTHON_FILES})
        SET(src "${VTK_SOURCE_DIR}/Wrapping/Python/${file}.py")
        SET(tgt "${VTK_BINARY_DIR}/Wrapping/Python/${config}/${file}.py")
        ADD_CUSTOM_COMMAND(DEPENDS ${src}
                           COMMAND ${CMAKE_COMMAND}
                           ARGS -E copy ${src} ${tgt}
                           OUTPUT ${tgt}
                           COMMENT "source copy")
      ENDFOREACH(file)
    ENDFOREACH(config)
  ENDIF(VTK_PYTHON_CONFIGURATION_TYPES)

  # Byte compile the Python files.
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/compile_all_vtk.py.in
                 ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
                 @ONLY IMMEDIATE)
  ADD_CUSTOM_COMMAND(
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    DEPENDS ${VTK_PYTHON_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    OUTPUT "${VTK_BINARY_DIR}/Wrapping/Python/vtk_compile_complete"
    )

  # Create a target on which custom commands can depend.  When this
  # is up-to-date, it is safe to run VTK python code.  Therefore
  # this should also depend on the vtkpython executable.
  ADD_CUSTOM_TARGET(vtkpython_pyc ALL echo "..."
    DEPENDS "${VTK_BINARY_DIR}/Wrapping/Python/vtk_compile_complete")
  ADD_DEPENDENCIES(vtkpython_pyc 
      vtkpython
      ${vtkpython_pyc_depends})

  # If no runtime is to be installed then do not install python modules.
  IF(VTK_INSTALL_NO_RUNTIME)
    SET(VTK_INSTALL_NO_PYTHON 1)
  ENDIF(VTK_INSTALL_NO_RUNTIME)

  # Add a rule to use python distutils to install the python wrappers.
  IF(NOT VTK_INSTALL_NO_PYTHON)
    SET(DOLLAR "$")
 
    # Create default python setup arguments if they are not set.
    IF(NOT DEFINED VTK_PYTHON_SETUP_ARGS)
      SET(VTK_PYTHON_SETUP_ARGS "--prefix=\"${DOLLAR}{CMAKE_INSTALL_PREFIX}\""
        CACHE STRING "Arguments passed to \"python setup.py install ...\" during installation.")
      MARK_AS_ADVANCED(VTK_PYTHON_SETUP_ARGS)
    ENDIF(NOT DEFINED VTK_PYTHON_SETUP_ARGS)

    # Change dollar sign to DOLLARSIGN
    STRING(REGEX REPLACE "\\${DOLLAR}" "DOLLARSIGN"
      PYTHON_SETUP_ARGS "${VTK_PYTHON_SETUP_ARGS}")

    # Get the install path for python modules
    EXEC_PROGRAM("${PYTHON_EXECUTABLE}" "${VTK_SOURCE_DIR}/Wrapping/Python"
      ARGS "setup_install_paths.py" "purelib" ${PYTHON_SETUP_ARGS}
      OUTPUT_VARIABLE PYTHON_MODULE_INSTALL_DIR)

    # Get the install path for binary python modules
    EXEC_PROGRAM("${PYTHON_EXECUTABLE}" "${VTK_SOURCE_DIR}/Wrapping/Python"
      ARGS "setup_install_paths.py" "platlib" ${PYTHON_SETUP_ARGS}
      OUTPUT_VARIABLE PYTHON_MODULE_PLATFORM_INSTALL_DIR)

    # Change DOLLARSIGN to dollar sign
    STRING(REGEX REPLACE "DOLLARSIGN" "${DOLLAR}"
      PYTHON_MODULE_INSTALL_DIR "${PYTHON_MODULE_INSTALL_DIR}")
    STRING(REGEX REPLACE "DOLLARSIGN" "${DOLLAR}"
      PYTHON_MODULE_PLATFORM_INSTALL_DIR
      "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}")

    # Replace backslashes with forward slashes
    IF (WIN32)
      STRING(REGEX REPLACE "\\\\" "/" PYTHON_MODULE_INSTALL_DIR
        "${PYTHON_MODULE_INSTALL_DIR}")
      STRING(REGEX REPLACE "\\\\" "/" PYTHON_MODULE_PLATFORM_INSTALL_DIR
        "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}")
    ENDIF (WIN32)

    SET(VTK_PYTHON_MODULE_INSTALL_DIR "${PYTHON_MODULE_INSTALL_DIR}"
      CACHE INTERNAL "Install directory for Python .py and .pyc files")

    SET(VTK_PYTHON_MODULE_PLATFORM_INSTALL_DIR
      "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}"
      CACHE INTERNAL "Install directory for Python binary modules")

    # If there are multiple configurations then add a BUILD_TYPE=...
    # argument to the python setup.py call.  The build type to use is set
    # in the CMake variable BUILD_TYPE while running the install script.
    IF(CMAKE_CONFIGURATION_TYPES)
      SET(VTK_PYTHON_SETUP_BUILD_TYPE "BUILD_TYPE=${DOLLAR}{BUILD_TYPE}")
    ELSE(CMAKE_CONFIGURATION_TYPES)
      SET(VTK_PYTHON_SETUP_BUILD_TYPE)
    ENDIF(CMAKE_CONFIGURATION_TYPES)

    # Configure the post-install script to run python on setup.py.
    CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/PythonInstall.cmake.in
                   ${VTK_BINARY_DIR}/Wrapping/Python/PythonInstall.cmake
                   @ONLY IMMEDIATE)
    SET_TARGET_PROPERTIES(vtkpython_pyc PROPERTIES POST_INSTALL_SCRIPT
      ${VTK_BINARY_DIR}/Wrapping/Python/PythonInstall.cmake
      )
  ENDIF(NOT VTK_INSTALL_NO_PYTHON)
ENDIF(PYTHON_EXECUTABLE)

# Create the setup.py file.
IF(CMAKE_CONFIGURATION_TYPES)
  # The build has multiple configuration types.  If CMAKE_BUILD_TYPE
  # is set use it as the default BUILD_TYPE for setup.py to install.
  SET(VTK_PYTHON_HAS_CONFIG_TYPES 1)
  IF(CMAKE_BUILD_TYPE)
    SET(VTK_PYTHON_BUILD_TYPE "\"${CMAKE_BUILD_TYPE}\"")
  ELSE(CMAKE_BUILD_TYPE)
    SET(VTK_PYTHON_BUILD_TYPE "[]")
  ENDIF(CMAKE_BUILD_TYPE)
ELSE(CMAKE_CONFIGURATION_TYPES)
  # The build has one configuration type.  The build type does not
  # affect installation.
  SET(VTK_PYTHON_HAS_CONFIG_TYPES 0)
  SET(VTK_PYTHON_BUILD_TYPE "[]")
ENDIF(CMAKE_CONFIGURATION_TYPES)
IF(VTK_USE_RENDERING)
  SET(VTK_PYTHON_USE_RENDERING 1)
ELSE(VTK_USE_RENDERING)
  SET(VTK_PYTHON_USE_RENDERING 0)
ENDIF(VTK_USE_RENDERING)
IF(VTK_USE_PARALLEL)
  SET(VTK_PYTHON_USE_PARALLEL 1)
ELSE(VTK_USE_PARALLEL)
  SET(VTK_PYTHON_USE_PARALLEL 0)
ENDIF(VTK_USE_PARALLEL)
IF(VTK_USE_INFOVIS)
  SET(VTK_PYTHON_USE_INFOVIS 1)
ELSE(VTK_USE_INFOVIS)
  SET(VTK_PYTHON_USE_INFOVIS 0)
ENDIF(VTK_USE_INFOVIS)
IF(VTK_USE_VIEWS)
  SET(VTK_PYTHON_USE_VIEWS 1)
ELSE(VTK_USE_VIEWS)
  SET(VTK_PYTHON_USE_VIEWS 0)
ENDIF(VTK_USE_VIEWS)
IF(VTK_USE_MPI)
  SET(VTK_PYTHON_USE_MPI 1)
ELSE(VTK_USE_MPI)
  SET(VTK_PYTHON_USE_MPI 0)
ENDIF(VTK_USE_MPI)
CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/setup.py.in
               ${VTK_BINARY_DIR}/Wrapping/Python/setup.py @ONLY IMMEDIATE)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
