set(classes
  vtkGL2PSExporter
  vtkOpenGLGL2PSExporter
  )

vtk_object_factory_declare(
  BASE vtkGL2PSExporter
  OVERRIDE vtkOpenGLGL2PSExporter)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKIOEXPORTGL2PS_EXPORT")

vtk_module_add_module(VTK::IOExportGL2PS
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
