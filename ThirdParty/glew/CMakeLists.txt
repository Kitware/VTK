if (ANDROID)
  set(VTK_MODULE_vtkglew_ANDROID 1)
endif()

if (NOT BUILD_SHARED_LIBS)
  set(GLEW_STATIC 1)
endif()

vtk_module_third_party(GLEW)
