# This is where we actually wrap the modules that have requested it.
include(vtkPythonWrapping)

get_property(VTK_PYTHON_MODULES GLOBAL PROPERTY VTK_PYTHON_WRAPPED)

# Get the include directories for the module and all its dependencies.
macro(vtk_include_recurse module)
  _vtk_module_config_recurse("${module}_PYTHON" ${module})
  include_directories(${${module}_PYTHON_INCLUDE_DIRS})
endmacro()

# Loop through all modules that should be wrapped, and wrap them.
foreach(module ${VTK_PYTHON_MODULES})
  vtk_module_load(${module})
  vtk_module_headers_load(${module})
  vtk_include_recurse(${module})
  vtk_add_python_wrapping(${module})
endforeach()

# Now to build the vtkPython executable, configure the .py files etc.
get_property(VTK_PYTHON_MODULES GLOBAL PROPERTY VTK_PYTHON_WRAPPED)

vtk_module_load(vtkWrappingPythonCore)
vtk_module_load(vtkPython)
include_directories(${CMAKE_CURRENT_BINARY_DIR}
  ${VTK_SOURCE_DIR}/Utilities
  ${vtkPython_INCLUDE_DIRS}
  ${vtkWrappingPythonCore_INCLUDE_DIRS})
# Needed for the vtkpython executable.
if(VTK_USE_CARBON)
  find_program(VTK_APPLE_RESOURCE Rez "${OSX_DEVELOPER_ROOT}/usr/bin")
endif()

if(UNIX)
  find_library(PYTHON_UTIL_LIBRARY
    NAMES util
    DOC "Utility library needed for vtkpython")
  mark_as_advanced(PYTHON_UTIL_LIBRARY)
endif()

# Generate the header which initializes Python modules when BUILD_SHARED_LIBS is
# OFF. The py_module_dependencies will be set to the libraries we should link
# against when we use the vtkpythonmodules.h file.
vtk_write_python_modules_header_for_wrapped_modules(
  "${CMAKE_CURRENT_BINARY_DIR}/vtkpythonmodules.h"
  py_module_dependencies)

# Create the VTK/Python  executable
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkPythonAppInitConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPythonAppInitConfigure.h)

add_executable(vtkpython vtkPythonAppInit.cxx vtkpython.rc)

unset(VTKPYTHON_LINK_FLAGS)
unset(VTKPYTHON_LINK_LIBS)

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
list(APPEND VTKPYTHON_LINK_LIBS vtkWrappingPythonCore)

set(vtkpython_pyc_depends
  vtkCommonPython
  vtkFilteringPython
  vtkIOPython
  vtkGraphicsPython
  vtkImagingPython
  )

if(PYTHON_UTIL_LIBRARY)
  list(APPEND VTKPYTHON_LINK_LIBS ${PYTHON_UTIL_LIBRARY})
endif()

set(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} ${py_module_dependencies})

# Link to rt to prevent undefined symbol 'fdatasync'
if(CMAKE_SYSTEM MATCHES "SunOS.*" AND NOT CMAKE_COMPILER_IS_GNUCXX)
  find_library(VTK_SUNCC_RT_LIBRARY rt)
  if(VTK_SUNCC_RT_LIBRARY)
    set(VTKPYTHON_LINK_LIBS ${VTKPYTHON_LINK_LIBS} ${VTK_SUNCC_RT_LIBRARY})
  endif()
endif()

# If we are wrapping Tcl then the Tk libraries are available and should be used
# in the Python wrapping too.
if(VTK_WRAP_TCL)
  list(APPEND VTKPYTHON_LINK_LIBS ${VTK_TK_LIBRARIES})
endif()

if(HAVE_PTHREAD_H AND CMAKE_USE_PTHREADS)
  list(APPEND VTKPYTHON_LINK_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

# FIXME: Is this needed here?
if(VTK_USE_FFMPEG_ENCODER)
  list(APPEND VTKPYTHON_LINK_LIBS ${FFMPEG_BASIC_LIBRARIES})
endif()

target_link_libraries(vtkpython ${VTKPYTHON_LINK_LIBS})

unset(PVTKPYTHON_EXECUTABLE)
# Create the pvtkpython Python wrapper executable with MPI support.
if(TARGET vtkParallelMPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_C_INCLUDE_PATH})
  set(MPI_LIBRARIES ${MPI_C_LIBRARIES})
  if(MPI_CXX_LIBRARIES)
    set(MPI_LIBRARIES ${MPI_LIBRARIES} ${MPI_CXX_LIBRARIES})
  endif()

  # Needed for mpich 2
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")

  set(PVTKPYTHON_EXECUTABLE pvtkpython)
  add_executable(pvtkpython vtkParaPythonAppInit.cxx)
  target_link_libraries(pvtkpython ${VTKPYTHON_LINK_LIBS} vtkParallelMPI
    ${MPI_LIBRARIES})

  if(VTK_USE_CARBON AND VTK_APPLE_RESOURCE)
    add_custom_command(TARGET pvtkpython POST_BUILD
      COMMAND ${VTK_APPLE_RESOURCE} Carbon.r -o ${VTK_EXECUTABLE_DIR}/pvtkpython)
  endif()
endif()

# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to VTK_BINARY_DIR if it is different from VTK_SOURCE_DIR.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***

if(PYTHON_EXECUTABLE)
  # Make the necessary directories.
  file(MAKE_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/gtk
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/qt4
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/tk
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/util
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/wx
    ${CMAKE_CURRENT_BINARY_DIR}/vtk/test)

# Now create a list of Python files.

# Wrapping/Python/vtk/*.py
  unset(VTK_PYTHON_FILES)
  unset(VTK_PYTHON_IMPORT_ALL)
  foreach(module ${VTK_PYTHON_MODULES})
    set(VTK_PYTHON_IMPORT_ALL "${VTK_PYTHON_IMPORT_ALL}from ${module} import *\n")
    configure_file(vtk/module.py.in vtk/${module}.py @ONLY IMMEDIATE)
  endforeach()
  configure_file(vtk/__init__.py.in vtk/__init__.py @ONLY IMMEDIATE)

# vtk.util package
  list(APPEND VTK_PYTHON_FILES
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
  list(APPEND VTK_PYTHON_FILES
    vtk/test/BlackBox
    vtk/test/Testing
    vtk/test/__init__
    )

# Tk related files
  list(APPEND VTK_PYTHON_FILES
    vtk/tk/__init__
    vtk/tk/vtkLoadPythonTkWidgets
    vtk/tk/vtkTkImageViewerWidget
    vtk/tk/vtkTkRenderWidget
    vtk/tk/vtkTkRenderWindowInteractor
    vtk/tk/vtkTkPhotoImage
    )

# PyQt v4 related files
  list(APPEND VTK_PYTHON_FILES
    vtk/qt4/__init__
    vtk/qt4/QVTKRenderWindowInteractor
    )

# wxPython related files
  list(APPEND VTK_PYTHON_FILES
    vtk/wx/__init__
    vtk/wx/wxVTKRenderWindow
    vtk/wx/wxVTKRenderWindowInteractor
    )

# pyGTK related files
  list(APPEND VTK_PYTHON_FILES
    vtk/gtk/GtkVTKRenderWindow
    vtk/gtk/__init__
    vtk/gtk/GtkVTKRenderWindowInteractor
    vtk/gtk/GtkGLExtVTKRenderWindow
    vtk/gtk/GtkGLExtVTKRenderWindowInteractor
    )

# Done listing of files.

# Now copy these files if necessary.
  unset(VTK_PYTHON_SOURCE_FILES)
  unset(VTK_PYTHON_OUTPUT_FILES)
  foreach(file ${VTK_PYTHON_FILES})
    set(src "${CMAKE_CURRENT_BINARY_DIR}/${file}.py")
    set(VTK_PYTHON_SOURCE_FILES ${VTK_PYTHON_SOURCE_FILES} ${src})
  endforeach()

  if(NOT "${VTK_BINARY_DIR}" MATCHES "^${VTK_SOURCE_DIR}$")
    foreach(file ${VTK_PYTHON_FILES})
      set(src "${CMAKE_CURRENT_SOURCE_DIR}/${file}.py")
      set(tgt "${CMAKE_CURRENT_BINARY_DIR}/${file}.py")
      add_custom_command(DEPENDS ${src}
        COMMAND ${CMAKE_COMMAND} -E copy ${src} ${tgt}
        OUTPUT ${tgt}
        COMMENT "source copy")
    endforeach()
  endif()

  # Byte compile the Python files.
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/compile_all_vtk.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    @ONLY IMMEDIATE)
  add_custom_command(
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    DEPENDS ${VTK_PYTHON_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vtk_compile_complete"
    )

  # Create a target on which custom commands can depend.  When this
  # is up-to-date, it is safe to run VTK python code.  Therefore
  # this should also depend on the vtkpython executable.
  add_custom_target(vtkpython_pyc ALL echo "..."
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/vtk_compile_complete")
  add_dependencies(vtkpython_pyc
    vtkpython
    ${vtkpython_pyc_depends})

  # If no runtime is to be installed then do not install python modules.
  if(VTK_INSTALL_NO_RUNTIME)
    set(VTK_INSTALL_NO_PYTHON 1)
  endif()

  # If set (generally by projects that include VTK), we install the
  # vtk{*}Python libraries using CMake rather than setup.py. Which implies that
  # they will be put will all other libraries. This is done in
  # KitCommonPythonWrapBlock.cmake
  if(VTK_INSTALL_PYTHON_USING_CMAKE)
    set(VTK_INSTALL_NO_PYTHON 1)
    # Install python module directory (*.py and *.pyd)
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vtk"
      DESTINATION "${VTK_INSTALL_RUNTIME_DIR}/Python" COMPONENT RuntimeLibraries
      USE_SOURCE_PERMISSIONS)
  endif()

  # Add a rule to use python distutils to install the python wrappers.
  if(NOT VTK_INSTALL_NO_PYTHON)
    set(DOLLAR "$")

    # Create default python setup arguments if they are not set.
    if(NOT DEFINED VTK_PYTHON_SETUP_ARGS)
      set(VTK_PYTHON_SETUP_ARGS "--prefix=\"${DOLLAR}{CMAKE_INSTALL_PREFIX}\""
        CACHE STRING "Arguments passed to \"python setup.py install ...\" during installation.")
      mark_as_advanced(VTK_PYTHON_SETUP_ARGS)
    endif()

    # Change dollar sign to DOLLARSIGN
    string(REGEX REPLACE "\\${DOLLAR}" "DOLLARSIGN"
      PYTHON_SETUP_ARGS "${VTK_PYTHON_SETUP_ARGS}")

    # Get the install path for python modules
    execute_process(COMMAND "${PYTHON_EXECUTABLE}"
        "setup_install_paths.py" "purelib" ${PYTHON_SETUP_ARGS}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE PYTHON_MODULE_INSTALL_DIR)

    # Get the install path for binary python modules
    execute_process(COMMAND "${PYTHON_EXECUTABLE}"
        "setup_install_paths.py" "platlib" ${PYTHON_SETUP_ARGS}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE PYTHON_MODULE_PLATFORM_INSTALL_DIR)

    # Change DOLLARSIGN to dollar sign
    string(REGEX REPLACE "DOLLARSIGN" "${DOLLAR}"
      PYTHON_MODULE_INSTALL_DIR "${PYTHON_MODULE_INSTALL_DIR}")
    string(REGEX REPLACE "DOLLARSIGN" "${DOLLAR}"
      PYTHON_MODULE_PLATFORM_INSTALL_DIR
      "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}")

    # Replace backslashes with forward slashes
    if(WIN32)
      string(REGEX REPLACE "\\\\" "/" PYTHON_MODULE_INSTALL_DIR
        "${PYTHON_MODULE_INSTALL_DIR}")
      string(REGEX REPLACE "\\\\" "/" PYTHON_MODULE_PLATFORM_INSTALL_DIR
        "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}")
    endif()

    set(VTK_PYTHON_MODULE_INSTALL_DIR "${PYTHON_MODULE_INSTALL_DIR}"
      CACHE INTERNAL "Install directory for Python .py and .pyc files")

    set(VTK_PYTHON_MODULE_PLATFORM_INSTALL_DIR
      "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}"
      CACHE INTERNAL "Install directory for Python binary modules")

    # If there are multiple configurations then add a BUILD_TYPE=...
    # argument to the python setup.py call.  The build type to use is set
    # in the CMake variable BUILD_TYPE while running the install script.
    if(CMAKE_CONFIGURATION_TYPES)
      set(VTK_PYTHON_SETUP_BUILD_TYPE "BUILD_TYPE=${DOLLAR}{BUILD_TYPE}")
    else()
      set(VTK_PYTHON_SETUP_BUILD_TYPE)
    endif()

    # Configure the post-install script to run python on setup.py.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/PythonInstall.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/PythonInstall.cmake
      @ONLY IMMEDIATE)

    install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/PythonInstall.cmake
      COMPONENT RuntimeLibraries)
  endif()
endif()

# Create the setup.py file.
if(CMAKE_CONFIGURATION_TYPES)
  # The build has multiple configuration types.  If CMAKE_BUILD_TYPE
  # is set use it as the default BUILD_TYPE for setup.py to install.
  set(VTK_PYTHON_HAS_CONFIG_TYPES 1)
  if(CMAKE_BUILD_TYPE)
    set(VTK_PYTHON_BUILD_TYPE "\"${CMAKE_BUILD_TYPE}\"")
  else()
    set(VTK_PYTHON_BUILD_TYPE "[]")
  endif()
else()
  # The build has one configuration type.  The build type does not
  # affect installation.
  set(VTK_PYTHON_HAS_CONFIG_TYPES 0)
  set(VTK_PYTHON_BUILD_TYPE "[]")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY IMMEDIATE)
