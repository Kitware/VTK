if(NOT VTK_OPENGL_HAS_OSMESA)
  if(VTK_DATA_ROOT)
    vtk_add_test_tcl(TestInteractorEventRecorder Widgets)
    vtk_add_test_tcl(TestSphereWidget Widgets)
  endif()

  vtk_add_test_tcl(TestBoxWidget Widgets)
  vtk_add_test_tcl(TestTextWidget Widgets)
endif()
