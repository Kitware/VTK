if (ANDROID)
  set(VTK_MODULE_vtkglew_ANDROID 1)
endif()

if (NOT BUILD_SHARED_LIBS)
  set(GLEW_STATIC 1)
endif()

include(CMakeDependentOption)
cmake_dependent_option(VTK_MODULE_vtkglew_IS_SHARED "Whether the glew in use is shared or not" ON
  VTK_USE_SYSTEM_GLEW "${BUILD_SHARED_LIBS}")
mark_as_advanced(VTK_MODULE_vtkglew_IS_SHARED)
if (VTK_MODULE_vtkglew_IS_SHARED)
  set(VTK_SYSTEM_GLEW_SHARED 1)
endif ()

vtk_module_third_party(GLEW)
