set(classes
  vtkPNetCDFPOPReader)

vtk_module_add_module(VTK::IOParallelNetCDF
  CLASSES ${classes})
