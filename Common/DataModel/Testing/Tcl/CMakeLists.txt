# Tests without test images
#
vtk_add_test_tcl(otherDataSetAttributes.tcl NO_DATA NO_RT)

# Tests with images
if(VTK_DATA_ROOT)
  vtk_add_test_tcl(headBone Graphics)
endif()

vtk_add_test_tcl(ImplicitSum Filtering)
vtk_add_test_tcl(PerlinNoise Filtering)
vtk_add_test_tcl(SelectionLoop Filtering)

vtk_add_test_tcl(LineIntersectQuadraticCells Graphics)
vtk_add_test_tcl(quadricCut Graphics)
vtk_add_test_tcl(scalarColors Graphics)
vtk_add_test_tcl(scalarConn Graphics)
vtk_add_test_tcl(TestConvexPointSet Graphics)
vtk_add_test_tcl(TestQuadricClustering Graphics)
vtk_add_test_tcl(TestStructuredGrid Graphics)

vtk_add_test_tcl(TestICPTransform Hybrid)
