set(classes
  vtkMPASReader
  vtkNetCDFAccessor
  vtkXArrayAccessor
  vtkNetCDFCAMReader
  vtkNetCDFCFReader
  vtkNetCDFCFWriter
  vtkNetCDFPOPReader
  vtkNetCDFReader
  vtkNetCDFUGRIDReader
  vtkSLACParticleReader
  vtkSLACReader)

vtk_module_add_module(VTK::IONetCDF
  CLASSES ${classes})
vtk_add_test_mangling(VTK::IONetCDF)
