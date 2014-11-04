# These tests are built under all conditions
set (GenericVolumeTclTests
  gaussian.tcl
  TestFixedPointRayCasterLinearCropped.tcl
  TestFixedPointRayCasterLinear.tcl
  TestFixedPointRayCasterNearestCropped.tcl
  TestFixedPointRayCasterNearest.tcl
  volRCClipPlanes.tcl
  volRCCropRegions.tcl
  volRCRotateClip.tcl
  VolumePickerCrop.tcl
  VolumePicker.tcl
  )

# These tests are only built when the rendering backend is OpenGL
set (VolumeOpenGLTclTests
  TestBunykRayCastFunction.tcl
  TestLODProp3D.tcl
  TestPTZSweep.tcl
  volTM2DCropRegions.tcl
  volTM2DRotateClip.tcl
  volTM3DCompressedCropRegions.tcl
  volTM3DCropRegions.tcl
  volTM3DRotateClip.tcl
  VolumeOutlineSourceClipped.tcl
  VolumeOutlineSource.tcl
  )

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_tcl(
    ${GenericVolumeTclTests}
    )
else ()
  vtk_add_test_tcl(
    ${GenericVolumeTclTests}
    ${VolumeOpenGLTclTests}
    )
endif ()
