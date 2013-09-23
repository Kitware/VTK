# Make sure we find the required Tcl components.
if(VTK_WRAP_TCL)
  set(VTK_WRAP_TCL_FIND_LIBS 1)
  include(vtkWrapTcl)
  include(vtkWrapHierarchy)
endif()

function(vtk_add_tcl_wrapping module_name module_srcs module_hdrs)
  if(NOT VTK_WRAP_TCL_INIT_EXE)
    if (TARGET vtkWrapTclInit)
      set(VTK_WRAP_TCL_INIT_EXE vtkWrapTclInit)
    else()
      message(FATAL_ERROR
        "VTK_WRAP_TCL_INIT_EXE must be set before calling vtk_add_tcl_wrapping.")
    endif()
  endif()
  # Need to add the Wrapping directory to the include directory
  set(_tcl_include_dirs
    ${VTK_SOURCE_DIR}/Wrapping/Tcl
    ${VTK_BINARY_DIR}/Wrapping/Tcl
    ${TCL_INCLUDE_PATH})

  if(NOT CMAKE_HAS_TARGET_INCLUDES)
    include_directories(${_tcl_include_dirs})
  endif()

  # FIXME: These must be here for now, should be fixed in the wrap hierarchy stuff
  if(NOT ${module_name}_EXCLUDE_FROM_WRAP_HIERARCHY)
    set(KIT_HIERARCHY_FILE ${${module_name}_WRAP_HIERARCHY_FILE})
  endif()

  string(REGEX REPLACE "^vtk" "" kit_name "${module_name}")
  set(KIT ${kit_name})

# FIXME: Terrible temporary hack - add in the extra source file for CommonCore
  if(${module_name} STREQUAL "vtkCommonCore")
     set(extra_srcs ${VTK_SOURCE_DIR}/Wrapping/Tcl/vtkTclUtil.cxx)
     set(extra_links vtksys)
  else()
    unset(extra_srcs)
    # This contains the vtkTclUtil class....
    set(extra_links vtkCommonCoreTCL)
  endif()

  # Figure out the dependent Tcl libraries for the module
  foreach(dep ${${vtk-module}_LINK_DEPENDS})
    if(NOT "${vtk-module}" STREQUAL "${dep}")
      if(NOT ${dep}_EXCLUDE_FROM_WRAPPING)
        list(APPEND extra_links ${${dep}_TCL_NAME}TCL)
      endif()
    endif()
  endforeach()

  # Tcl will not accept module names with numbers in.
  set(tcl_module ${${module_name}_TCL_NAME})
  vtk_wrap_tcl3(${tcl_module}TCL Tcl_SRCS "${module_srcs}" "")
  vtk_add_library(${tcl_module}TCL ${Tcl_SRCS} ${extra_srcs})
  if(CMAKE_HAS_TARGET_INCLUDES)
    set_property(TARGET ${tcl_module}TCL APPEND
      PROPERTY INCLUDE_DIRECTORIES ${_tcl_include_dirs})
  endif()
  if(${module_name}_IMPLEMENTS)
    set_property(TARGET ${tcl_module}TCL PROPERTY COMPILE_DEFINITIONS
      "${module_name}_AUTOINIT=1(${module_name})")
  endif()
  target_link_libraries(${tcl_module}TCL LINK_PUBLIC ${module_name}
    ${extra_links} ${VTK_TCL_LIBRARIES})
endfunction()
