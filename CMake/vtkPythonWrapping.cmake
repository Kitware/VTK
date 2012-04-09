find_package(PythonLibs REQUIRED)
include(vtkWrapPython)

function(vtk_add_python_wrapping module_name module_srcs)
  if(NOT VTK_WRAP_PYTHON_INIT_EXE)
    message(FATAL_ERROR "VTK must be built with Python wrapping turned on.")
  endif()
  # Need to add the Wrapping/Python to the include directory
  include_directories(
    ${VTK_SOURCE_DIR}/Wrapping/PythonCore
    ${VTK_BINARY_DIR}/Wrapping/PythonCore
    ${VTK_SOURCE_DIR}/Wrapping
    ${VTK_BINARY_DIR}/Wrapping
    ${PYTHON_INCLUDE_DIRS})

  # FIXME: These must be here for now, should be fixed in the wrap hierarchy stuff
  set(KIT_HIERARCHY_FILE ${CMAKE_CURRENT_BINARY_DIR}/${module_name}Hierarchy.txt)
  string(REGEX REPLACE "^vtk" "" kit_name "${module_name}")
  set(KIT ${kit_name})

  # FIXME: Terrible temporary hack - add in the extra source file for CommonCore
  if(${module_name} STREQUAL "vtkCommonCore")
    set(extra_srcs vtkPythonCommand.cxx)
    unset(extra_links)
  else()
    unset(extra_srcs)
    # This contains the PyVTKClass....
    set(extra_links vtkCommonCorePythonD)
  endif()

  # Figure out the dependent PythonD libraries for the module
  foreach(dep ${VTK_MODULE_${vtk-module}_DEPENDS})
    if(NOT "${vtk-module}" STREQUAL "${dep}")
      if(NOT VTK_MODULE_${dep}_EXCLUDE_FROM_WRAPPING)
        list(APPEND extra_links ${dep}PythonD)
        list(APPEND VTK_WRAP_INCLUDE_DIRS ${${dep}_SOURCE_DIR})
      endif()
    endif()
  endforeach()

  vtk_wrap_python3(${module_name}Python Python_SRCS "${module_srcs}")
  vtk_add_library(${module_name}PythonD ${Python_SRCS} ${extra_srcs})
  if(VTK_MODULE_${module_name}_IMPLEMENTS)
    set_property(TARGET ${module_name}PythonD PROPERTY COMPILE_DEFINITIONS
      "${module_name}_AUTOINIT=1(${module_name})")
  endif()
  target_link_libraries(${module_name}PythonD ${module_name}
    vtkWrappingPythonCore ${extra_links} ${VTK_PYTHON_LIBRARIES})
  python_add_module(${module_name}Python ${module_name}PythonInit.cxx)
  if(PYTHON_ENABLE_MODULE_${module_name}Python)
    target_link_libraries(${module_name}Python ${module_name}PythonD)
  endif()
endfunction()
