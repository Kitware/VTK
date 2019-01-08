set(classes
  vtkPExodusIIReader
  vtkPExodusIIWriter)

vtk_object_factory_declare(
  BASE      vtkExodusIIWriter
  OVERRIDE  vtkPExodusIIWriter)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKIOPARALLELEXODUS_EXPORT")

vtk_module_add_module(VTK::IOParallelExodus
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
