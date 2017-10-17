# These tests are built under all conditions
set (GenericVolumePythonTests
     TestFixedPointRayCasterLinearCropped.py,NO_RT
     TestFixedPointRayCasterLinear.py,NO_RT
     TestFixedPointRayCasterNearestCropped.py,NO_RT
     TestFixedPointRayCasterNearest.py,NO_RT
     cursor3D.py
     VolumePicker.py,NO_RT
     VolumePickerCrop.py
    )

# These tests are only built when the rendering backend is OpenGL
set (VolumeOpenGLPythonTests
     TestBunykRayCastFunction.py
     TestPTZSweep.py
     TestLODProp3D.py
     VolumeOutlineSourceClipped.py
     VolumeOutlineSource.py
)

# These tests are only built when the rendering backend is OpenGL2
set (VolumeOpenGL2PythonTests
     gaussian.py
     TestGPURayCastIndependentComponent.py,NO_RT
     TestGPURayCastIndependentComponentMIP.py,NO_RT
     TestGPURayCastIndependentComponentMinIP.py,NO_RT
     TestGPURayCastUserShader.py
     )

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_python(
    ${GenericVolumePythonTests}
    ${VolumeOpenGL2PythonTests}
    )
else ()
  vtk_add_test_python(
    ${GenericVolumePythonTests}
    ${VolumeOpenGLPythonTests}
    )
endif ()
