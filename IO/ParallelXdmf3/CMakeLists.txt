set(classes
  vtkPXdmf3Writer)

vtk_object_factory_declare(
  BASE vtkXdmf3Writer
  OVERRIDE vtkPXdmf3Writer)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKIOPARALLELXDMF3_EXPORT")

vtk_module_add_module(VTK::IOParallelXdmf3
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
