if(NOT VTK_OPENGL_HAS_OSMESA)
  if(VTK_DATA_ROOT)
    vtk_add_test_tcl(TestInteractorEventRecorder)
    vtk_add_test_tcl(TestSphereWidget)
  endif()

  vtk_add_test_tcl(TestBoxWidget)
  vtk_add_test_tcl(TestTextWidget)
endif()
