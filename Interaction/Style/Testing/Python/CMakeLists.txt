if (VTK_CAN_DO_ONSCREEN AND NOT VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  vtk_add_test_python(
    TestFlyTo.py
    TestStyleRubberBandZoom.py
    TestStyleRubberBandZoomPerspective.py,NO_RT
    TestInteractorStyleTerrain.py
    TestStyleBaseSpike.py,NO_RT
    TestStyleJoystickActor.py,NO_RT
    TestStyleJoystickCamera.py,NO_RT
    TestStyleTerrain.py,NO_RT
    TestStyleTrackballActor.py,NO_RT
    TestStyleTrackballCamera.py,NO_RT
    TestStyleUser.py
    )

  set_tests_properties(vtkInteractionStylePython-TestStyleRubberBandZoomPerspective
                       PROPERTIES RUN_SERIAL ON)
endif ()
