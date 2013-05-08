# Tests without test images
#
add_test(NAME ${vtk-module}Tcl-otherDataSetAttributes
         COMMAND ${VTK_TCL_EXE}
         ${CMAKE_CURRENT_SOURCE_DIR}/otherDataSetAttributes.tcl
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

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
