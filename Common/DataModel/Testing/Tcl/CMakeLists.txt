# Tests without test images
#
add_test(NAME ${vtk-module}Tcl-otherDataSetAttributes
         COMMAND ${VTK_TCL_EXE}
         ${CMAKE_CURRENT_SOURCE_DIR}/otherDataSetAttributes.tcl
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

# Tests with images
if(VTK_DATA_ROOT)
  add_test_tcl(headBone Graphics)
endif()

add_test_tcl(ImplicitSum Filtering)
add_test_tcl(PerlinNoise Filtering)
add_test_tcl(SelectionLoop Filtering)

add_test_tcl(LineIntersectQuadraticCells Graphics)
add_test_tcl(quadricCut Graphics)
add_test_tcl(scalarColors Graphics)
add_test_tcl(scalarConn Graphics)
add_test_tcl(TestConvexPointSet Graphics)
add_test_tcl(TestQuadricClustering Graphics)
add_test_tcl(TestStructuredGrid Graphics)

add_test_tcl(TestICPTransform Hybrid)
