set(vtkutf8_LIBRARIES "")

vtk_module_impl()

vtk_module_export_info()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_utf8.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}"
    COMPONENT   Development)
endif ()
