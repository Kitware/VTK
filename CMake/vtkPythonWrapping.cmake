find_package(PythonLibs REQUIRED)
include(vtkWrapPython)

function(vtk_add_python_wrapping module_name)
  if(NOT VTK_WRAP_PYTHON_INIT_EXE)
    if(TARGET vtkWrapPythonInit)
      set (VTK_WRAP_PYTHON_INIT_EXE vtkWrapPythonInit)
    else()
      message(FATAL_ERROR "VTK must be built with Python wrapping turned on.")
    endif()
  endif()
  # Need to add the Wrapping/Python to the include directory
  set(_python_include_dirs
    ${VTK_SOURCE_DIR}/Wrapping/Python
    ${VTK_BINARY_DIR}/Wrapping/Python
    ${VTK_SOURCE_DIR}/Utilities/Python
    ${VTK_BINARY_DIR}/Utilities/Python
    ${PYTHON_INCLUDE_DIRS})

  if(NOT CMAKE_HAS_TARGET_INCLUDES)
    include_directories(${_python_include_dirs})
  endif()

  if(NOT ${module_name}_EXCLUDE_FROM_WRAP_HIERARCHY)
    set(KIT_HIERARCHY_FILE ${${module_name}_WRAP_HIERARCHY_FILE})
  endif()

  string(REGEX REPLACE "^vtk" "" kit_name "${module_name}")
  set(KIT ${kit_name})
  set(XY ${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION})

  # Figure out the dependent PythonXYD libraries for the module
  unset(extra_links)
  set(EXTRA_PYTHON_INCLUDE_DIRS ${${module_name}_PYTHON_INCLUDE_DIRS})
  foreach(dep ${${module_name}_DEPENDS})
    if(NOT "${module_name}" STREQUAL "${dep}" AND TARGET ${dep}PythonD)
      list(APPEND extra_links ${dep}PythonD)
    endif()
  endforeach()

  if(${module_name}_WRAP_HINTS AND EXISTS "${${module_name}_WRAP_HINTS}")
    set(VTK_WRAP_HINTS "${${module_name}_WRAP_HINTS}")
  endif()

  vtk_wrap_python(${module_name}Python Python_SRCS ${module_name})
  vtk_add_library(${module_name}PythonD ${Python_SRCS} ${extra_srcs})
  get_property(output_name TARGET ${module_name}PythonD PROPERTY OUTPUT_NAME)
  string(REPLACE "PythonD" "Python${XY}D" output_name "${output_name}")
  set_property(TARGET ${module_name}PythonD PROPERTY OUTPUT_NAME ${output_name})
  if(CMAKE_HAS_TARGET_INCLUDES)
    set_property(TARGET ${module_name}PythonD APPEND
      PROPERTY INCLUDE_DIRECTORIES ${_python_include_dirs})
  endif()
  if(${module_name}_IMPLEMENTS)
    set_property(TARGET ${module_name}PythonD PROPERTY COMPILE_DEFINITIONS
      "${module_name}_AUTOINIT=1(${module_name})")
  endif()
  target_link_libraries(${module_name}PythonD ${module_name}
    vtkWrappingPythonCore ${extra_links} ${VTK_PYTHON_LIBRARIES})

  _vtk_add_python_module(${module_name}Python ${module_name}PythonInit.cxx)
  target_link_libraries(${module_name}Python ${module_name}PythonD)
  if(CMAKE_HAS_TARGET_INCLUDES)
    set_property(TARGET ${module_name}Python APPEND
      PROPERTY INCLUDE_DIRECTORIES ${_python_include_dirs})
  endif()
endfunction()

#------------------------------------------------------------------------------
# _vtk_add_python_module(<name> src1 src2..) is used to build modules for Python.
# A python module is the module that gets imported in Python interpretor.
# This is an internal function used by vtk_add_python_wrapping() to create the
# module for each VTK module. If BUILD_SHARED_LIBS is OFF, this simply creates
# a static library. In that case applications are expected to use
# vtk_write_python_modules_header_for_wrapped_modules() to generate a header
# that statically initializes the modules after Py_Inititalize() is called.
function(_vtk_add_python_module name)
  if (BUILD_SHARED_LIBS)
    add_library(${name} MODULE ${ARGN})
    set_property(TARGET ${name} PROPERTY PREFIX "${PYTHON_MODULE_PREFIX}")
    if (VTK_INSTALL_PYTHON_USING_CMAKE)
      # if setup.py is not being used to install python modules, we need to
      # add install rules for them.
      vtk_target_install(${name})
    endif()
    if (WIN32 AND NOT CYGWIN)
      # when building shared on Windows, the python module files need to be
      # named as *.pyd
      set_target_properties(${name} PROPERTIES SUFFIX ".pyd")
    endif()
  else ()
    # when building statically, the module targets need to be exported since
    # others can link against them, unlike when building shared, and hence we
    # use vtk_add_library() call.
    vtk_add_library(${name} ${ARGN})
  endif()
endfunction()

#------------------------------------------------------------------------------
# vtk_write_python_modules_header(filename vtk-module1 vtk-module2...)
# This is similar to vtk_write_python_modules_header_for_wrapped_modules()
# execpt caller is expected to explitictly specify the modules to be initialized
# in the generated header. The header file will have non-empty content only if
# BUILD_SHARED_LIBS is OFF. When ON, the header file contains empty functions
# and hence can still be used by the client code, but it doesn't really do any
# initialization.
function(vtk_write_python_modules_header filename)
  get_filename_component(_name "${filename}" NAME_WE)
  STRING(REPLACE "." "_" _name "${_name}")
  STRING(TOUPPER ${_name} _nameUpper)

  set (EXTERN_DEFINES)
  set (INIT_CALLS)

  if (NOT BUILD_SHARED_LIBS)
    # fill in the init functions only when BUILD_SHARED_LIBS is OFF.
    foreach (module ${ARGN})
      set (EXTERN_DEFINES "${EXTERN_DEFINES}\n  extern void init${module}Python();")
      set (INIT_CALLS "${INIT_CALLS}\n
  static char name${module}[] = \"${module}Python\";
  PyImport_AppendInittab(name${module}, init${module}Python);")
    endforeach()
  endif()

  configure_file(${VTK_CMAKE_DIR}/pythonmodules.h.in
    ${filename} @ONLY)
endfunction()

#------------------------------------------------------------------------------
# create init header for all python wrapped modules.
# this uses VTK_PYTHON_WRAPPED global property which is filled with every pyhton
# module.
# Usage: vtk_write_python_modules_header_for_wrapped_modules(
#           <filename> <out_variable>)
# out_variable is set to the list of dependencies to which the code including
# the header file should link against (using target_link_libraries()).
function(vtk_write_python_modules_header_for_wrapped_modules filename out_var)
  get_property(python_wrapped_modules GLOBAL PROPERTY VTK_PYTHON_WRAPPED)
  vtk_write_python_modules_header(
    "${filename}" ${python_wrapped_modules})
  set (dependencies)
  if (NOT BUILD_SHARED_LIBS)
    foreach(mod IN LISTS python_wrapped_modules)
      list(APPEND dependencies ${mod}Python)
    endforeach()
  endif()
  set (${out_var} "${dependencies}" PARENT_SCOPE)
endfunction()
