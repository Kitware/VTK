# Tests with test images in Baseline/Hybrid
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    camera
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

# Tests with test images in Baseline/Graphics
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    contour3DAll
    eleState
    extrudeCopyCD
    Hyper
    HyperScalarBar
    KlineBottle
    schwarz
    smoothCyl
    smoothCyl2
    subdividePointData
    subDivideTetra
    sync3dAll
    TenEllip
    TestBandedContourFilter
    TestBoxFunction
    TestFillHolesFilter
    TestImageMarchingCubes
    TestRibbonAndTube
    TestRuledSurface
    TestRuledSurface2
    TestSpherePuzzle
    TestSpherePuzzleArrows
    contour2DAll
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
