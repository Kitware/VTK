set(classes
  vtkMPASReader
  vtkNetCDFCAMReader
  vtkNetCDFCFReader
  vtkNetCDFPOPReader
  vtkNetCDFReader
  vtkSLACParticleReader
  vtkSLACReader)

vtk_module_add_module(VTK::IONetCDF
  CLASSES ${classes})
