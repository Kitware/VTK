set(classes
  vtkMatplotlibMathTextUtilities)

vtk_object_factory_declare(
  BASE      vtkMathTextUtilities
  OVERRIDE  vtkMatplotlibMathTextUtilities)
vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGMATPLOTLIB_EXPORT")

vtk_module_add_module(VTK::RenderingMatplotlib
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})

set_property(GLOBAL APPEND
  PROPERTY
    vtk_required_python_modules "matplotlib>=2.0.0")
