# These tests are built under all conditions
set (GenericVolumeTclTests
  TestFixedPointRayCasterLinearCropped.tcl
  TestFixedPointRayCasterLinear.tcl
  TestFixedPointRayCasterNearestCropped.tcl
  TestFixedPointRayCasterNearest.tcl
  )

if (NOT VTK_LEGACY_REMOVE)
  list (APPEND GENERICVolumeTclTests
    gaussian.tcl
  volRCClipPlanes.tcl
  volRCCropRegions.tcl
  volRCRotateClip.tcl
  VolumePickerCrop.tcl
  VolumePicker.tcl
  )
endif()

# These tests are only built when the rendering backend is OpenGL
set (VolumeOpenGLTclTests
  TestBunykRayCastFunction.tcl
  TestPTZSweep.tcl
  )

if (NOT VTK_LEGACY_REMOVE)
  list (APPEND VolumeOpenGLTclTests
    TestLODProp3D.tcl
    volTM2DCropRegions.tcl
    volTM2DRotateClip.tcl
    volTM3DCompressedCropRegions.tcl
    volTM3DCropRegions.tcl
    volTM3DRotateClip.tcl
    VolumeOutlineSourceClipped.tcl
    VolumeOutlineSource.tcl
    )
endif()

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
