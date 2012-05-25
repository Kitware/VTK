# Tests with test images in Baseline/Graphics
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    CellDataToPointData
    Delaunay2D
    Delaunay2DAlpha
    Delaunay2DTransform
    Delaunay3D
    QuadricDecimation
    StreamPolyData
    TestGridSynchronizedTemplates3D
    TestMarchingSquares
    TestRectilinearSynchronizedTemplates
    TestSynchronizedTemplates2D
    TestSynchronizedTemplates3D
    TestTriangleFilter
    capCow
    capSphere
    clipArt
    combStreamers
    combStreamers2
    constrainedDelaunay
    contourCells
    contourQuadraticCells
    createBFont
    cutLoop
    deciFranFace
    deciPlane
    dispPlot
    fieldToPolyData
    fieldToRGrid
    fieldToSGrid
    fieldToUGrid
    financialField
    hull
    mergeFilter
    multipleComponentContour
    multipleIso
    polyConn
    probe
    probeComb
    reverseNormals
    skinOrder
    smoothMeshOnMesh
    stripF
    teapotHulls
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
    MassProperties
    financialField2
    financialField3
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
