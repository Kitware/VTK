vtk_object_factory_declare(
  BASE vtkFreeTypeTools
  OVERRIDE vtkFontConfigFreeTypeTools)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGFREETYPEFONTCONFIG_EXPORT")

set(classes
  vtkFontConfigFreeTypeTools)

vtk_module_add_module(VTK::RenderingFreeTypeFontConfig
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})

vtk_module_find_package(PACKAGE FontConfig)
vtk_module_link(VTK::RenderingFreeTypeFontConfig
  PRIVATE
    FontConfig::FontConfig)
