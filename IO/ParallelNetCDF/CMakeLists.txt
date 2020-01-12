set(classes
  vtkPNetCDFPOPReader
  vtkPSLACReader)

vtk_object_factory_declare(
  BASE      vtkSLACReader
  OVERRIDE  vtkPSLACReader)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKIOPARALLELNETCDF_EXPORT")

vtk_module_add_module(VTK::IOParallelNetCDF
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
