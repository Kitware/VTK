vtk_object_factory_declare(
  BASE      vtkTextRenderer
  OVERRIDE  vtkMathTextFreeTypeTextRenderer)
vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKRENDERINGFREETYPE_EXPORT")

set(classes
  vtkFreeTypeStringToImage
  vtkFreeTypeTools
  vtkMathTextFreeTypeTextRenderer
  vtkMathTextUtilities
  vtkScaledTextActor
  vtkTextRendererStringToImage
  vtkVectorText)

set(sources
  ${vtk_object_factory_source}

  fonts/face_arial_bold.cxx
  fonts/face_arial_bold_italic.cxx
  fonts/face_arial.cxx
  fonts/face_arial_italic.cxx
  fonts/face_courier_bold.cxx
  fonts/face_courier_bold_italic.cxx
  fonts/face_courier.cxx
  fonts/face_courier_italic.cxx
  fonts/face_times_bold.cxx
  fonts/face_times_bold_italic.cxx
  fonts/face_times.cxx
  fonts/face_times_italic.cxx)

set(private_headers
  ${vtk_object_factory_header})

vtk_module_add_module(VTK::RenderingFreeType
  CLASSES         ${classes}
  SOURCES         ${sources}
  PRIVATE_HEADERS ${private_headers})
