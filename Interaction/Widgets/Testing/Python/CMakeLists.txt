include(vtkOpenGLOptions)
if(vtk_can_do_onscreen AND NOT VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  vtk_add_test_python(
    TestCameraOrientationWidget.py
    TestImagePlaneWidget.py
    TestBoxWidget.py
    TestImplicitImageRepresentation.py
    TestInteractivePlaneCutter.py
    TestInteractorEventRecorder.py
    TestMagnifierWidget.py
    TestPointCloudWidget.py
    TestPointCloudWidget2.py
    TestSphereWidget.py
    TestTensorWidget.py
    TestTensorWidget2.py
    TestTextWidget.py
    )
endif()
