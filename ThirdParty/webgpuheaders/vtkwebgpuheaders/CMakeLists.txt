vtk_module_install_headers(
  SUBDIR "vtkwebgpuheaders/include/webgpu"
  FILES include/webgpu/webgpu.h)

vtk_module_include(VTK::webgpuheaders
  INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkwebgpuheaders/include>")
