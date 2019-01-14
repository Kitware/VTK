set(classes
  vtkPNrrdReader
  vtkMPIImageReader)

# Now to generate our object factory.
vtk_object_factory_declare(
  BASE      vtkNrrdReader
  OVERRIDE  vtkPNrrdReader)
vtk_object_factory_declare(
  BASE      vtkImageReader
  OVERRIDE  vtkMPIImageReader)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKIOMPIIMAGE_EXPORT")

vtk_module_add_module(VTK::IOMPIImage
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
