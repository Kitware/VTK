vtk_module_install_headers(
  SUBDIR "vtkwebgpuheaders/include/dawn"
  FILES
    include/dawn/webgpu.h
    include/dawn/webgpu_cpp.h
    include/dawn/webgpu_cpp_print.h)

vtk_module_install_headers(
  SUBDIR "vtkwebgpuheaders/include/webgpu"
  FILES
    include/webgpu/webgpu.h
    include/webgpu/webgpu_cpp.h
    include/webgpu/webgpu_cpp_chained_struct.h
    include/webgpu/webgpu_cpp_print.h
    include/webgpu/webgpu_enum_class_bitmasks.h)

# Both webgpu/ (redirect wrappers) and dawn/ (actual content) must be reachable
# from the same include root so the include chain resolves correctly.
vtk_module_include(VTK::webgpuheaders
  INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkwebgpuheaders/include>")
