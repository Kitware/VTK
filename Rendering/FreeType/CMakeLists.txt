find_package(FontConfig QUIET)

if(FONTCONFIG_FOUND)
  include_directories(${FONTCONFIG_INCLUDE_DIR})
endif()

set(Module_SRCS
  vtkFreeTypeStringToImage.cxx
  vtkFreeTypeUtilities.cxx
  vtkFreeTypeTools.cxx
  vtkTextActor3D.cxx
  vtkTextActor.cxx
  vtkScaledTextActor.cxx
  vtkVectorText.cxx

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

if(FONTCONFIG_FOUND)
  set_property(SOURCE vtkFreeTypeTools.cxx
               PROPERTY COMPILE_DEFINITIONS FONTCONFIG_FOUND)
endif()

vtk_module_library(${vtk-module} ${Module_SRCS})

if(FONTCONFIG_FOUND)
  target_link_libraries(${vtk-module} ${FONTCONFIG_LIBRARIES})
endif()
