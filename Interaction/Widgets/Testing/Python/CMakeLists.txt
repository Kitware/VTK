if(NOT VTK_OPENGL_HAS_OSMESA)
  vtk_add_test_python(
    TestImagePlaneWidget.py
    TestBoxWidget.py,NO_RT
    TestInteractorEventRecorder.py,NO_RT
    TestSphereWidget.py,NO_RT
    TestTextWidget.py,NO_RT
    )
endif()
