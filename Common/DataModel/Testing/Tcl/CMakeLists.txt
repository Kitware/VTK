# Tests without test images
#
add_test(NAME ${vtk-module}Tcl-otherDataSetAttributes
         COMMAND ${VTK_TCL_EXE}
         ${CMAKE_CURRENT_SOURCE_DIR}/otherDataSetAttributes.tcl
         -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

# Tests with test images in Baseline/Filtering
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    ImplicitSum
    PerlinNoise
    SelectionLoop
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Filtering/${tfile}.png
    -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
endforeach()

# Tests with test images in Baseline/Graphics
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    headBone
    LineIntersectQuadraticCells
    quadricCut
    scalarColors
    scalarConn
    TestConvexPointSet
    TestQuadricClustering
    TestStructuredGrid
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Graphics/${tfile}.png
    -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
endforeach()

# Tests with test images in Baseline/Hybrid
#
add_test(NAME ${vtk-module}Tcl-TestICPTransform COMMAND ${VTK_TCL_EXE}
  ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
  ${CMAKE_CURRENT_SOURCE_DIR}/TestICPTransform.tcl
  -D ${VTK_DATA_ROOT}
  -T ${VTK_TEST_OUTPUT_DIR}
  -V Baseline/Hybrid/TestICPTransform.png
  -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)
