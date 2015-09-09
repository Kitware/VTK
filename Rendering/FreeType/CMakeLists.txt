include(vtkObjectFactory)

set(Module_SRCS
  vtkFreeTypeStringToImage.cxx
  vtkFreeTypeTools.cxx
  vtkMathTextFreeTypeTextRenderer.cxx
  vtkMathTextUtilities.cxx
  vtkTextRendererStringToImage.cxx
  vtkScaledTextActor.cxx
  vtkVectorText.cxx

  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx

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
  fonts/face_times_italic.cxx
  )

set_source_files_properties(
  vtkFreeTypeTools
  WRAP_EXCLUDE
)

set_source_files_properties(
  fonts/face_arial_bold
  fonts/face_arial_bold_italic
  fonts/face_arial
  fonts/face_arial_italic
  fonts/face_courier_bold
  fonts/face_courier_bold_italic
  fonts/face_courier
  fonts/face_courier_italic
  fonts/face_times_bold
  fonts/face_times_bold_italic
  fonts/face_times
  fonts/face_times_italic
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

# Override some rendering classes. Create object factory:
list(APPEND vtk_module_overrides "vtkTextRenderer")
set(vtk_module_vtkTextRenderer_override "vtkMathTextFreeTypeTextRenderer")
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(${vtk-module} ${Module_SRCS})
