if(VTK_CAN_DO_ONSCREEN AND NOT VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  vtk_add_test_python(
    TestImagePlaneWidget.py
    TestBoxWidget.py,NO_RT
    TestInteractivePlaneCutter.py
    TestInteractorEventRecorder.py,NO_RT
    TestSphereWidget.py,NO_RT
    TestTextWidget.py,NO_RT
    )
endif()
