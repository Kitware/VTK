set(classes
  vtkPDFContextDevice2D
  vtkPDFExporter)

vtk_object_factory_declare(
  BASE vtkContextDevice2D
  OVERRIDE vtkPDFContextDevice2D)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKIOEXPORTPDF_EXPORT")

vtk_module_add_module(VTK::IOExportPDF
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
