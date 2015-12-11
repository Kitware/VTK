# These tests are built under all conditions
set (GenericVolumePythonTests
     TestFixedPointRayCasterLinearCropped.py,NO_RT
     TestFixedPointRayCasterLinear.py,NO_RT
     TestFixedPointRayCasterNearestCropped.py,NO_RT
     TestFixedPointRayCasterNearest.py,NO_RT)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND GenericVolumePythonTests
    cursor3D.py
    gaussian.py
    volRCClipPlanes.py
    volRCCropRegions.py
    volRCRotateClip.py
    VolumePicker.py,NO_RT
    VolumePickerCrop.py)
endif()

# These tests are only built when the rendering backend is OpenGL
set (VolumeOpenGLPythonTests
     TestBunykRayCastFunction.py
     TestPTZSweep.py)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND VolumeOpenGLPythonTests
    TestLODProp3D.py
    volTM2DRotateClip.py
    volTM2DCropRegions.py
    volTM3DCropRegions.py
    volTM3DCompressedCropRegions.py
    volTM3DRotateClip.py
    VolumeOutlineSourceClipped.py
    VolumeOutlineSource.py)
endif()

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
  if (NOT VTK_LEGACY_REMOVE)
    set_tests_properties(vtkRenderingVolumePython-volTM3DCompressedCropRegions
      PROPERTIES TIMEOUT 250)
    set_tests_properties(vtkRenderingVolumePython-volTM3DCropRegions
      PROPERTIES TIMEOUT 250)
    set_tests_properties(vtkRenderingVolumePython-volTM3DRotateClip
      PROPERTIES TIMEOUT 80)
  endif()
endif ()
