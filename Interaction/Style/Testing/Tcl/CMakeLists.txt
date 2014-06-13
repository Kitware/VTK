if(NOT VTK_OPENGL_HAS_OSMESA)
  vtk_add_test_tcl(
    TestInteractorStyleTerrain.tcl
    TestStyleRubberBandZoom.tcl

    TestStyleJoystickCamera.tcl
    TestFlyTo.tcl
    TestStyleTerrain.tcl
    TestStyleTrackballActor.tcl
    TestStyleBaseSpike.tcl
    TestStyleTrackballCamera.tcl
    TestStyleJoystickActor.tcl
    )
endif()
