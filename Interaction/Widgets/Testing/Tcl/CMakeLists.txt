if(NOT VTK_OPENGL_HAS_OSMESA)
  vtk_add_test_tcl(
    TestInteractorEventRecorder.tcl
    TestSphereWidget.tcl
    TestBoxWidget.tcl
    TestTextWidget.tcl
    )
endif()
