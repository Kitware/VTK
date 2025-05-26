set(headers
  vtkEmbindSmartPointerTrait.h)

vtk_module_add_module(VTK::WrappingJavaScript
  HEADER_ONLY
  HEADERS ${headers})
