# TODO: Update FindJNI to use imported targets.
find_package(JNI REQUIRED)

set(classes
  vtkJavaUtil)

set(headers
  vtkJavaAwt.h)

vtk_module_add_module(VTK::Java
  CLASSES ${classes}
  HEADERS ${headers})
vtk_module_include(VTK::Java SYSTEM
  PUBLIC
    ${JNI_INCLUDE_DIRS})
vtk_module_link(VTK::Java
  PUBLIC
    ${JNI_LIBRARIES})

  if (NOT VTK_ABI_NAMESPACE_NAME STREQUAL "<DEFAULT>" AND NOT DEFINED ENV{CI})
  message(WARNING "Utilties::Java ABI does not not support the VTK_ABI_NAMESPACE_NAME "
                  "and the symbols will not be mangled.")
endif ()
