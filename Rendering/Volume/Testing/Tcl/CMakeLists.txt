# These tests are built under all conditions
set (GenericVolumeTclTests
  TestFixedPointRayCasterLinearCropped.tcl
  TestFixedPointRayCasterLinear.tcl
  TestFixedPointRayCasterNearestCropped.tcl
  TestFixedPointRayCasterNearest.tcl
  )

if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  vtk_add_test_tcl(
    ${GenericVolumeTclTests}
    )
endif ()
