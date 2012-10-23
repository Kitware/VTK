set(vtkTclTk_THIRD_PARTY 1)
set(vtkTckTk_LIBRARIES)
vtk_module_export_info()

# Set if the tcl/tk libs are static
option(VTK_TCL_TK_STATIC "Build with static Tcl/Tk support. TCL_LIBRARY and TK_LIBRARY must point to the corresponding Tcl/Tk static libraries (example, tcl85sx.lib, tk85sx.lib)." OFF)
mark_as_advanced(VTK_TCL_TK_STATIC)

INCLUDE(vtkDependentOption)

# Use Tk Toolkit (with Tcl or Python)
set(VTK_CAN_USE_TK OFF)
set(VTK_USE_TK_DEFAULT ON)
if(VTK_WRAP_PYTHON OR VTK_WRAP_TCL)
  if(NOT VTK_DISABLE_TK_INIT)
    set(VTK_CAN_USE_TK ON)
    if(APPLE)
      if(OSX_SDK_VERSION)
        if(${OSX_SDK_VERSION} VERSION_LESS "10.6")
          # Until OS X 10.6, building Tk with Cocoa was not possible
          # (VTK_USE_COCOA is set in Rendering/OpenGL, but will be cached)
          if(VTK_USE_COCOA)
            set(VTK_CAN_USE_TK OFF)
          endif()
        else()
          # For OS X 10.6, the default system Tk is Cocoa, not Carbon
          # (VTK_USE_COCOA is set in Rendering/OpenGL, but will be cached)
          if(VTK_USE_CARBON)
            set(VTK_USE_TK_DEFAULT OFF)
          endif()
        endif()
      endif()
    endif()
  endif()
endif()
vtk_dependent_option(VTK_USE_TK "Build VTK with Tk support"
                     ${VTK_USE_TK_DEFAULT} "VTK_CAN_USE_TK" OFF)

# The cmake code to find the libs is in vtkWrapTcl
set(VTK_WRAP_TCL_FIND_LIBS 1)
include(vtkWrapTcl)

# Subdirs
ADD_SUBDIRECTORY(internals)
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(resources)
