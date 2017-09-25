# These tests are built under all conditions
set (GenericVolumeTclTests
  TestFixedPointRayCasterLinearCropped.tcl
  TestFixedPointRayCasterLinear.tcl
  TestFixedPointRayCasterNearestCropped.tcl
  TestFixedPointRayCasterNearest.tcl
  )

# These tests are only built when the rendering backend is OpenGL
set (VolumeOpenGLTclTests
  TestBunykRayCastFunction.tcl
  TestPTZSweep.tcl
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
