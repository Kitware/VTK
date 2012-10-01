# Tests with test images in Baseline/Graphics
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    Canny
    OBBCylinder
    TestCellDerivs
    TestClipClosedSurface
    TestClipOutline
    TestCurvatures
    TestDiscreteMarchingCubes
    TestGraphLayoutFilter
    TestMultiBlockStreamer
    TestRandomAttributeGenerator
    TestRectilinearGridToTetrahedra
    TestSplineFilter
    WarpScalarImage
    WarpToImage
    WarpVectorImage
    blankGrid
    clipComb
    clipImage
    clipQuadraticCells
    clipVolume
    clipVolume2
    clipVolume3
    contoursToSurface
    cursor2D
    cursor3D
    dicer
    edgePoints
    mcubes
    recursiveDC
    spatialRepAll
    splitVectors
    streamTracer
    subPixelPositionMin
    testDataSetTriangleFilter
    testDataSetTriangleFilter2
    testReflect
    warplens
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Graphics/${tfile}.png
   )
endforeach()

# Tests with test images in Baseline/Hybrid
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    clipHex
    clipPyramid
    clipTet
    clipWedge
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Hybrid/${tfile}.png
   )
endforeach()
