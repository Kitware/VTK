find_package(PythonLibs REQUIRED)
include(vtkWrapPython)
if(PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND)
  set(_interp_version "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
  set(_libs_version "${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}")
  if(NOT ${_interp_version} STREQUAL ${_libs_version})
    message(WARNING "Python library ${_libs_version} mismatches python ${_interp_version}.")
   endif()
endif()

# To support wrapping of either module or kit, this function
# has two signatures:
# 1) vtk_add_python_wrapping(<module_name> <sources_var>)
# 2) vtk_add_python_wrapping("<module_name>[ <module_name>]" <sources_var> <kit_name>)
#
# Legacy code may call this function with a single argument. In that case,
# vtk_add_python_wrapping_library() is called internally to maintain backwards
# compatibility.
function(vtk_add_python_wrapping module_names)
  if(${ARGC} EQUAL 1)
    set(_legacy TRUE)
    message(AUTHOR_WARNING
      "Calling vtk_add_python_wrapping() with a single argument is deprecated.\n"
      "Replace calls like:\n"
      "    vtk_add_python_wrapping(\${module})\n"
      "with:\n"
      "    vtk_add_python_wrapping(\${module} module_srcs)\n"
      "    vtk_add_python_wrapping_library(\${module} module_srcs \${module})")
  endif()
  if("${ARGV1}" MATCHES ".+")
    set(sources_var ${ARGV1})
  endif()
  if("${ARGV2}" MATCHES ".+")
    list(REMOVE_AT ARGN 0)
    set(target ${ARGN})
  else()
    set(target ${module_names})
  endif()
  if(NOT VTK_WRAP_PYTHON_INIT_EXE)
    if(TARGET vtkWrapPythonInit)
      set (VTK_WRAP_PYTHON_INIT_EXE vtkWrapPythonInit)
    else()
      message(FATAL_ERROR "VTK must be built with Python wrapping turned on.")
    endif()
  endif()

  set(EXTRA_PYTHON_INCLUDE_DIRS)
  set(KIT_HIERARCHY_FILE)
  set(VTK_WRAP_HINTS_FILES)

  foreach(module_name IN LISTS module_names)
    list(APPEND EXTRA_PYTHON_INCLUDE_DIRS ${${module_name}_PYTHON_INCLUDE_DIRS})

    if(NOT ${module_name}_EXCLUDE_FROM_WRAP_HIERARCHY)
      list(APPEND KIT_HIERARCHY_FILE ${${module_name}_WRAP_HIERARCHY_FILE})
    endif()

    if(${module_name}_WRAP_HINTS AND EXISTS "${${module_name}_WRAP_HINTS}")
      list(APPEND VTK_WRAP_HINTS_FILES "${${module_name}_WRAP_HINTS}")
    endif()
  endforeach()

  if(VTK_WRAP_HINTS_FILES)
    set(VTK_WRAP_HINTS ${VTK_WRAP_HINTS_FILES})
  endif()

  vtk_wrap_python(${target}Python Python_SRCS "${module_names}")
  if(_legacy)
    set(_sources "${Python_SRCS}" "${extra_srcs}")
    vtk_add_python_wrapping_library(${module_names} _sources ${module_names})
  else()
    set(${sources_var} "${Python_SRCS}" "${extra_srcs}" PARENT_SCOPE)
  endif()
endfunction()

function(vtk_add_python_wrapping_library module srcs)
  # Need to add the Wrapping/Python to the include directory
  set(_python_include_dirs
    ${VTK_SOURCE_DIR}/Wrapping/Python
    ${VTK_BINARY_DIR}/Wrapping/Python
    ${VTK_SOURCE_DIR}/Utilities/Python
    ${VTK_BINARY_DIR}/Utilities/Python
    ${PYTHON_INCLUDE_DIRS})
  set(XY ${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION})

  if(NOT CMAKE_HAS_TARGET_INCLUDES)
    include_directories(${_python_include_dirs})
  endif()

  # Figure out the dependent PythonXYD libraries for the module
  set(extra_links)
  string(REPLACE "Kit" "" kit_basename "${module}")
  if (_${kit_basename}_is_kit)
    set(${module}_WRAP_DEPENDS)
    foreach (kit_module IN LISTS _${kit_basename}_modules)
      list(APPEND ${module}_WRAP_DEPENDS
        ${${kit_module}_WRAP_DEPENDS})
    endforeach ()
  endif ()
  foreach(dep IN LISTS ${module}_WRAP_DEPENDS)
    if (module STREQUAL dep OR kit_basename STREQUAL dep)
      continue ()
    endif ()

    if (VTK_ENABLE_KITS AND ${dep}_KIT)
      if (NOT ${dep}_KIT STREQUAL kit_basename)
        list(APPEND extra_links ${${dep}_KIT}KitPythonD)
      endif ()
    elseif (TARGET ${dep}PythonD)
      list(APPEND extra_links ${dep}PythonD)
    endif ()
  endforeach()

  vtk_add_library(${module}PythonD ${${srcs}})
  get_property(output_name TARGET ${module}PythonD PROPERTY OUTPUT_NAME)
  string(REPLACE "PythonD" "Python${XY}D" output_name "${output_name}")
  set_property(TARGET ${module}PythonD PROPERTY OUTPUT_NAME ${output_name})
  if(CMAKE_HAS_TARGET_INCLUDES)
    set_property(TARGET ${module}PythonD APPEND
      PROPERTY INCLUDE_DIRECTORIES ${_python_include_dirs})
  endif()
  target_link_libraries(${module}PythonD LINK_PUBLIC
    vtkWrappingPythonCore ${extra_links} ${VTK_PYTHON_LIBRARIES})

  if (MSVC)
    set_target_properties(${module}PythonD
      PROPERTIES STATIC_LIBRARY_FLAGS ${CMAKE_MODULE_LINKER_FLAGS})
  endif()

  foreach (submodule IN LISTS ARGN)
    if(${submodule}_IMPLEMENTS)
      set_property(TARGET ${module}PythonD APPEND PROPERTY COMPILE_DEFINITIONS
        "${submodule}_AUTOINIT=1(${submodule})")
    endif()
    target_link_libraries(${module}PythonD LINK_PUBLIC ${submodule})
  endforeach ()

  set(prefix ${module})
  if(_${module}_is_kit)
    set(prefix ${prefix}${VTK_KIT_SUFFIX})
  endif()
  _vtk_add_python_module(${module}Python ${prefix}PythonInit.cxx)
  target_link_libraries(${module}Python ${module}PythonD)
  if(CMAKE_HAS_TARGET_INCLUDES)
    set_property(TARGET ${module}Python APPEND
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
  if(VTK_ENABLE_KITS)
    # Process VTK_PYTHON_WRAPPED to generate a list of Python wrapped kits and
    # modules.
    set(_kits_and_modules)
    foreach(module IN LISTS python_wrapped_modules)
      vtk_module_load("${module}")
      if(${module}_KIT)
        set(kit ${${module}_KIT})
        list(APPEND _kits_and_modules ${kit}Kit)
      else()
        list(APPEND _kits_and_modules ${module})
      endif()
    endforeach()
    list(REMOVE_DUPLICATES _kits_and_modules)
    set(python_wrapped_modules ${_kits_and_modules})
  endif()
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
