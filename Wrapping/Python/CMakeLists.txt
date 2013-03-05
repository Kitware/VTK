include(GenerateExportHeader)
include(vtkPythonWrapping)

set(Module_SRCS
  vtkPythonArgs.cxx
  vtkPythonCommand.cxx
  vtkPythonOverload.cxx
  vtkPythonUtil.cxx
  PyVTKClass.cxx
  PyVTKMutableObject.cxx
  PyVTKObject.cxx
  PyVTKSpecialObject.cxx
  PyVTKTemplate.cxx
  )

set(Module_HDRS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Module.h
  )

list(APPEND Module_SRCS ${Module_HDRS})

set_source_files_properties(
  ${Module_HDRS}
  HEADER_FILE_ONLY
  )

include_directories(
  ${VTK_SOURCE_DIR}/Utilities
  )

# SIP directory will be searched before other python dirs
SET(PYTHON_WRAPPING_INCLUDE_DIRS)
if(VTK_WRAP_PYTHON_SIP)
  list(APPEND PYTHON_WRAPPING_INCLUDE_DIRS "${SIP_INCLUDE_DIR}")
endif()
set(${vtk-module}_SYSTEM_INCLUDE_DIRS ${PYTHON_WRAPPING_INCLUDE_DIRS})

set(${vtk-module}_NO_HeaderTest 1)
set(${vtk-module}_INCLUDE_DIRS)

set(XY ${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION})
vtk_module_library(${vtk-module} ${Module_SRCS})
get_property(output_name TARGET ${vtk-module} PROPERTY OUTPUT_NAME)
string(REPLACE "Python" "Python${XY}" output_name "${output_name}")
set_property(TARGET ${vtk-module} PROPERTY OUTPUT_NAME ${output_name})

get_property(VTK_PYTHON_MODULES GLOBAL PROPERTY VTK_PYTHON_WRAPPED)

# If we are using instantiators then configure them all into a header.
unset(vtk_instantiators)
unset(vtk_link_modules)
if(VTK_MAKE_INSTANTIATORS)
  foreach(module ${VTK_PYTHON_MODULES})
    set(vtk_instantiators
      "${vtk_instantiators}#include \"${module}Instantiator.h\"\n")
    list(APPEND vtk_link_modules ${module})
    include_directories("${${module}_BINARY_DIR}")
  endforeach()
endif()
# Make a nice empty file if not using instantiators.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/vtkInstantiators.h.in"
  "@vtk_instantiators@\n")
configure_file("${CMAKE_CURRENT_BINARY_DIR}/vtkInstantiators.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkInstantiators.h" @ONLY)

#-----------------------
# for the vtkpython executable
if(VTK_USE_CARBON)
  find_program(VTK_APPLE_RESOURCE Rez /Developer/Tools)
endif()

if(UNIX)
  find_library(PYTHON_UTIL_LIBRARY
    NAMES util
    DOC "Utility library needed for vtkpython"
    )
  mark_as_advanced(PYTHON_UTIL_LIBRARY)
endif()

# generate the header which initializes Python modules when BUILD_SHARED_LIBS is
# OFF. py_module_dependencies will be set to the libraries we should link
# against when we use the vtkpythonmodules.h file.
vtk_write_python_modules_header_for_wrapped_modules(
  "${CMAKE_CURRENT_BINARY_DIR}/vtkpythonmodules.h"
  py_module_dependencies)

# create the VTK/Python  executable
configure_file(${VTK_SOURCE_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h.in
               ${VTK_BINARY_DIR}/Wrapping/Python/vtkPythonAppInitConfigure.h)

add_executable(vtkpython vtkPythonAppInit.cxx vtkpython.rc)

set(VTKPYTHON_LINK_FLAGS)
set(VTKPYTHON_LINK_LIBS  ${vtk_link_modules})

if(CMAKE_SYSTEM_NAME MATCHES "AIX")
  get_filename_component(CMAKE_PYTHON_LIB_PREFIX "${PYTHON_LIBRARY}" PATH)
  find_file(CMAKE_PYTHON_LIBRARY_EXPORT python.exp "${CMAKE_PYTHON_LIB_PREFIX}")
  if(CMAKE_PYTHON_LIBRARY_EXPORT)
    set(VTKPYTHON_LINK_FLAGS "-Wl,-bE:${CMAKE_PYTHON_LIBRARY_EXPORT}")
  endif()
endif()

if(APPLE)
  if(VTK_USE_CARBON AND VTK_APPLE_RESOURCE)
    get_target_property(
      VTKPYTHON_RUNTIME_DIR vtkpython RUNTIME_OUTPUT_DIRECTORY)
    add_custom_command(
      TARGET vtkpython POST_BUILD
      COMMAND ${VTK_APPLE_RESOURCE} Carbon.r -o ${VTKPYTHON_RUNTIME_DIR}/vtkpython
      )
  endif()

  if(VTK_USE_CARBON)
    # MacOS python has a --enable-toolbox-glue option that is on by default,
    # which adds the "-u _PyMac_Error" linker flag to force inclusion of some
    # Carbon extension functions for Python.  It is irrelevant for Cocoa VTK.
    set(VTKPYTHON_LINK_FLAGS "-u _PyMac_Error")
  endif()
endif()

# Link against all the kit wrappers.
list(APPEND VTKPYTHON_LINK_LIBS
  ${vtk-module}
  )

set(vtkpython_pyc_depends
  vtkCommonPython
  vtkFilteringPython
  vtkIOPython
  vtkGraphicsPython
  vtkImagingPython
  )

if(PYTHON_UTIL_LIBRARY)
  set(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} ${PYTHON_UTIL_LIBRARY})
endif()

set(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} ${py_module_dependencies})

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

IF(HAVE_PTHREAD_H AND CMAKE_USE_PTHREADS)
  TARGET_LINK_LIBRARIES(vtkpython ${CMAKE_THREAD_LIBS_INIT})
ENDIF()

IF(VTK_USE_FFMPEG_ENCODER)
  LIST(APPEND VTKPYTHON_LINK_LIBS ${FFMPEG_BASIC_LIBRARIES})
ENDIF(VTK_USE_FFMPEG_ENCODER)

TARGET_LINK_LIBRARIES(vtkpython ${VTKPYTHON_LINK_LIBS})

IF(VTK_USE_MATLAB_MEX)
  INCLUDE(${MATLAB_MEX_USE_FILE})
  SET_TARGET_PROPERTIES(vtkpython PROPERTIES
                        LINK_FLAGS "${MEX_RPATH} ${VTKPYTHON_LINK_FLAGS}")
ELSE(VTK_USE_MATLAB_MEX)
  SET_TARGET_PROPERTIES(vtkpython PROPERTIES LINK_FLAGS "${VTKPYTHON_LINK_FLAGS}")
ENDIF(VTK_USE_MATLAB_MEX)


SET(PVTKPYTHON_EXECUTABLE)
# Create the pvtkpython Python wrapper executable with MPI support.
IF (VTK_USE_PARALLEL AND VTK_USE_MPI)
  FIND_PACKAGE(MPI REQUIRED)
  INCLUDE_DIRECTORIES("${MPI_C_INCLUDE_PATH}")
  set (MPI_LIBRARIES ${MPI_C_LIBRARIES})
  if (MPI_CXX_LIBRARIES)
    SET(MPI_LIBRARIES ${MPI_LIBRARIES} ${MPI_CXX_LIBRARIES})
  endif()
  IF (VTK_USE_PARALLEL_BGL)
    SET(MPI_LIBRARIES ${MPI_LIBRARIES} "${Boost_LIBRARIES}")
  ENDIF (VTK_USE_PARALLEL_BGL)

  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")

  SET(PVTKPYTHON_EXECUTABLE pvtkpython)
  VTK_ADD_EXECUTABLE(pvtkpython vtkParaPythonAppInit.cxx)
  TARGET_LINK_LIBRARIES(pvtkpython ${VTKPYTHON_LINK_LIBS} ${MPI_LIBRARIES})
  IF(VTK_USE_MATLAB_MEX)
    INCLUDE(${MATLAB_MEX_USE_FILE})
    SET_TARGET_PROPERTIES(pvtkpython PROPERTIES
                         LINK_FLAGS "${MEX_RPATH} ${VTKPYTHON_LINK_FLAGS}")
  ELSE(VTK_USE_MATLAB_MEX)
    SET_TARGET_PROPERTIES(pvtkpython PROPERTIES LINK_FLAGS "${VTKPYTHON_LINK_FLAGS}")
  ENDIF(VTK_USE_MATLAB_MEX)

  IF(VTK_USE_CARBON AND VTK_APPLE_RESOURCE)
      ADD_CUSTOM_COMMAND( TARGET pvtkpython POST_BUILD
                          COMMAND ${VTK_APPLE_RESOURCE} Carbon.r -o ${VTK_EXECUTABLE_DIR}/pvtkpython )
  ENDIF(VTK_USE_CARBON AND VTK_APPLE_RESOURCE)
ENDIF (VTK_USE_PARALLEL AND VTK_USE_MPI)

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
    )
  SET(VTK_PYTHON_IMPORT_ALL "")
  FOREACH(module ${VTK_PYTHON_MODULES})
    SET(VTK_PYTHON_IMPORT_ALL "${VTK_PYTHON_IMPORT_ALL}from ${module} import *\n")
    CONFIGURE_FILE(vtk/module.py.in vtk/${module}.py @ONLY IMMEDIATE)
  ENDFOREACH()
  CONFIGURE_FILE(vtk/__init__.py.in vtk/__init__.py @ONLY IMMEDIATE)

# vtk.util package
  SET(VTK_PYTHON_FILES
    ${VTK_PYTHON_FILES}
    vtk/util/__init__
    vtk/util/vtkMethodParser
    vtk/util/misc
    vtk/util/vtkConstants
    vtk/util/vtkImageExportToArray
    vtk/util/vtkImageImportFromArray
    vtk/util/vtkVariant
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
    vtk/gtk/GtkGLExtVTKRenderWindow
    vtk/gtk/GtkGLExtVTKRenderWindowInteractor
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

  # If set (generally by projects that include VTK), we install the
  # vtk{*}Python libraries using CMake rather than setup.py. Which implies that
  # they will be put will all other libraries. This is done in
  # KitCommonPythonWrapBlock.cmake
  IF (VTK_INSTALL_PYTHON_USING_CMAKE)
    SET(VTK_INSTALL_NO_PYTHON 1)
  ENDIF (VTK_INSTALL_PYTHON_USING_CMAKE)

  IF(VTK_INSTALL_PYTHON_USING_CMAKE)
    # Install python module directory (*.py and *.pyd)
    #INSTALL(DIRECTORY "${VTK_BINARY_DIR}/Wrapping/Python/vtk"
    #  DESTINATION "${VTK_INSTALL_BIN_DIR_CM24}/Python" COMPONENT RuntimeLibraries
    #  USE_SOURCE_PERMISSIONS)
  ENDIF(VTK_INSTALL_PYTHON_USING_CMAKE)

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
    execute_process(COMMAND "${PYTHON_EXECUTABLE}"
        "setup_install_paths.py" "purelib" ${PYTHON_SETUP_ARGS}
      WORKING_DIRECTORY "${VTK_SOURCE_DIR}/Wrapping/Python"
      OUTPUT_VARIABLE PYTHON_MODULE_INSTALL_DIR)

    # Get the install path for binary python modules
    execute_process(COMMAND "${PYTHON_EXECUTABLE}"
        "setup_install_paths.py" "platlib" ${PYTHON_SETUP_ARGS}
      WORKING_DIRECTORY "${VTK_SOURCE_DIR}/Wrapping/Python"
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

    install(SCRIPT ${VTK_BINARY_DIR}/Wrapping/Python/PythonInstall.cmake
            COMPONENT RuntimeLibraries
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

CONFIGURE_FILE(${VTK_SOURCE_DIR}/Wrapping/Python/setup.py.in
               ${VTK_BINARY_DIR}/Wrapping/Python/setup.py @ONLY IMMEDIATE)

# Allow the user to customize their build with some local options
#
INCLUDE (${VTK_BINARY_DIR}/Wrapping/Python/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/Wrapping/Python/LocalUserOptions.cmake OPTIONAL)
