set(classes
  QVTKWebGPUWidget)

include(vtkQt)
set(qt_components
  Widgets)

# Determine the minimum Qt version required.
if (VTK_USE_X)
  set(_qt_min_version 6.0)
else()
  set(_qt_min_version 5.9)
endif()

vtk_module_find_package(
  PACKAGE "Qt${vtk_qt_major_version}"
  VERSION ${_qt_min_version}
  COMPONENTS ${qt_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR "Qt${vtk_qt_major_version}_VERSION")

# Verify that Qt meets the requirement when using X11.
if (VTK_USE_X AND Qt${vtk_qt_major_version}_VERSION VERSION_LESS "6.0")
  message(FATAL_ERROR
    "QVTKWebGPUWidget on X11 requires Qt >= 6.0. "
    "Detected Qt${vtk_qt_major_version}_VERSION=${Qt${vtk_qt_major_version}_VERSION}.")
endif()

set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::GUISupportQtWebGPU
  CLASSES ${classes})

foreach(_qt_comp IN LISTS qt_components)
  list(APPEND qt_modules "Qt${vtk_qt_major_version}::${_qt_comp}")
endforeach()

vtk_module_definitions(VTK::GUISupportQtWebGPU PRIVATE QT_NO_KEYWORDS)
vtk_module_link(VTK::GUISupportQtWebGPU PUBLIC ${qt_modules})

# QVTKWebGPUWidget builds the WebGPU surface descriptor from the native window
# handle, so it needs to know which windowing system is in use. Only the flag is
# needed: Qt hands out the native handles and Dawn takes them as opaque
# void*/uint64_t, so neither libX11 nor libwayland-client is a compile-time or
# link-time dependency of this module.
if(VTK_USE_Wayland)
  vtk_module_definitions(VTK::GUISupportQtWebGPU PRIVATE VTK_USE_Wayland=1)
elseif(VTK_USE_X)
  vtk_module_definitions(VTK::GUISupportQtWebGPU PRIVATE VTK_USE_X=1)
endif()

vtk_add_test_mangling(VTK::GUISupportQtWebGPU)
