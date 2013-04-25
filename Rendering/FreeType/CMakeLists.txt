set(Module_SRCS
  vtkFreeTypeStringToImage.cxx
  vtkFreeTypeUtilities.cxx
  vtkFreeTypeTools.cxx
  vtkMathTextFreeTypeTextRenderer.cxx
  vtkMathTextUtilities.cxx
  vtkTextActor3D.cxx
  vtkTextActor.cxx
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
  vtkFreeTypeUtilities
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
  WRAP_EXCLUDE
  )

# Override some rendering classes. Create object factory:
list(APPEND vtk_module_overrides "vtkTextRenderer")
set(vtk_module_vtkTextRenderer_override "vtkMathTextFreeTypeTextRenderer")

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now let's create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(${vtk-module} ${Module_SRCS})
