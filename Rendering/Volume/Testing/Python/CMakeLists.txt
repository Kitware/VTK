# These tests are built under all conditions
set (GenericVolumePythonTests
     cursor3D.py
     gaussian.py
     TestFixedPointRayCasterLinearCropped.py,NO_RT
     TestFixedPointRayCasterLinear.py,NO_RT
     TestFixedPointRayCasterNearestCropped.py,NO_RT
     TestFixedPointRayCasterNearest.py,NO_RT
     volRCClipPlanes.py
     volRCCropRegions.py
     volRCRotateClip.py
     VolumePickerCrop.py
     VolumePicker.py,NO_RT)

# These tests are only built when the rendering backend is OpenGL
set (VolumeOpenGLPythonTests
     TestBunykRayCastFunction.py
     TestLODProp3D.py
     TestPTZSweep.py
     volTM2DCropRegions.py
     volTM2DRotateClip.py
     volTM3DCompressedCropRegions.py
     volTM3DCropRegions.py
     volTM3DRotateClip.py
     VolumeOutlineSourceClipped.py
     VolumeOutlineSource.py)

# These tests are only built when the rendering backend is OpenGL2
set (VolumeOpenGL2PythonTests
     TestGPURayCastIndependentComponent.py,NO_RT
     TestGPURayCastIndependentComponentMIP.py,NO_RT
     TestGPURayCastIndependentComponentMinIP.py,NO_RT)

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
  set_tests_properties(vtkRenderingVolumePython-volTM3DCompressedCropRegions
    PROPERTIES TIMEOUT 250)
  set_tests_properties(vtkRenderingVolumePython-volTM3DCropRegions
    PROPERTIES TIMEOUT 250)
  set_tests_properties(vtkRenderingVolumePython-volTM3DRotateClip
    PROPERTIES TIMEOUT 80)
endif ()
