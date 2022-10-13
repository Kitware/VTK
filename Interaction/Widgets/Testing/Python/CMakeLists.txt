include(vtkOpenGLOptions)
if(vtk_can_do_onscreen AND NOT VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  vtk_add_test_python(
    TestCameraOrientationWidget.py,NO_RT
    TestImagePlaneWidget.py
    TestBoxWidget.py,NO_RT
    TestImplicitImageRepresentation.py
    TestInteractivePlaneCutter.py
    TestInteractorEventRecorder.py,NO_RT
    TestMagnifierWidget.py
    TestPointCloudWidget.py
    TestPointCloudWidget2.py
    TestSphereWidget.py,NO_RT
    TestTensorWidget.py
    TestTensorWidget2.py
    TestTextWidget.py,NO_RT
    )
endif()
