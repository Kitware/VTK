# Add X11 include dirs and check for Xcursor presence if VTK_USE_X is ON.
if (VTK_USE_X)
  vtk_module_find_package(PACKAGE X11) # for X11 headers
  if (NOT X11_FOUND)
    message(FATAL_ERROR "VTK_USE_X is ON but X11 not found!")
  endif()
endif ()

# Define the VTK X related configuration options in a header.
set(VTK_HAVE_XCURSOR 0)
if (TARGET X11::Xcursor)
  set(VTK_HAVE_XCURSOR 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkX11Functions.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkX11Functions.h")
vtk_module_add_module(VTK::x11
  HEADERS "${CMAKE_CURRENT_BINARY_DIR}/vtkX11Functions.h"
  SOURCES vtkX11Functions.cxx)

# Add X11 include dirs.
if (VTK_USE_X)
  # XXX(cmake-3.27): $<COMPILE_ONLY>
  if (CMAKE_VERSION VERSION_LESS "3.27")
    if (VTK_RELOCATABLE_INSTALL)
      message(WARNING
        "CMake 3.27+ is required to make a relocatable build with deferred "
        "loading of X11 libraries")
    endif ()
    # Use include paths.
    vtk_module_include(VTK::x11
      PUBLIC ${X11_INCLUDE_DIR})
  else ()
    # Propagate X11 compilation requirements.
    vtk_module_link(VTK::x11
      PUBLIC "$<COMPILE_ONLY:X11::X11>")
  endif ()
endif ()
