if(NOT VTK_OPENGL_HAS_OSMESA AND NOT VTK_USE_OFFSCREEN_EGL)
  vtk_add_test_python(
    TestImagePlaneWidget.py
    TestBoxWidget.py,NO_RT
    TestInteractivePlaneCutter.py
    TestInteractorEventRecorder.py,NO_RT
    TestSphereWidget.py,NO_RT
    TestTextWidget.py,NO_RT
    )
endif()
