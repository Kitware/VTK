set(headers
  vtkAffineImplicitBackend.h
  vtkConstantImplicitBackend.h
)

vtk_module_add_module(VTK::CommonImplicitBackends
  HEADER_ONLY
  HEADERS ${headers}
)
