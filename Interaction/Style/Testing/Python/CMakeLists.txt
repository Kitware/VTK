if(NOT VTK_OPENGL_HAS_OSMESA AND NOT VTK_USE_OFFSCREEN_EGL)
  vtk_add_test_python(
    TestFlyTo.py
    TestStyleRubberBandZoom.py
    TestInteractorStyleTerrain.py
    TestStyleBaseSpike.py,NO_RT
    TestStyleJoystickActor.py,NO_RT
    TestStyleJoystickCamera.py,NO_RT
    TestStyleTerrain.py,NO_RT
    TestStyleTrackballActor.py,NO_RT
    TestStyleTrackballCamera.py,NO_RT
    )
endif()
