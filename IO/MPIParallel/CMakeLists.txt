set(classes
  vtkPWindBladeReader
  vtkMPIMultiBlockPLOT3DReader)

vtk_object_factory_declare(
  BASE      vtkWindBladeReader
  OVERRIDE  vtkPWindBladeReader)
vtk_object_factory_declare(
  BASE      vtkMultiBlockPLOT3DReader
  OVERRIDE  vtkMPIMultiBlockPLOT3DReader)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKIOMPIPARALLEL_EXPORT")

vtk_module_add_module(VTK::IOMPIParallel
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
