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
