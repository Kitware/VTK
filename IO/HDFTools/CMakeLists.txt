set(no_wrap_headers
  vtkHDF5ScopedHandle.h)

vtk_module_add_module(VTK::IOHDFTools
  NOWRAP_HEADERS ${no_wrap_headers}
  HEADER_ONLY)
