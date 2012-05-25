# Tests with test images in Baseline/Graphics
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    dataSetSurfaceFilter
    geomFilter
    officeStreamPoints
    rectGrid
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
