set(classes
  QVTKWebGPUWidget)

include(vtkQt)
set(qt_components
  Widgets)

vtk_module_find_package(
  PACKAGE     "Qt${vtk_qt_major_version}"
  VERSION     5.9
  COMPONENTS  ${qt_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         "Qt${vtk_qt_major_version}_VERSION")

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
