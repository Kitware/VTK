# Add tests, with the data
#
unset(tests)

if(VTK_DATA_ROOT)
  list(APPEND tests
    CSpline
    KSpline
    closedSplines
    )
endif()

foreach( tfile ${tests})
  set(_test_args
    ${vtkTestingRendering_SOURCE_DIR}/rtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Filtering/${tfile}.png
    -A ${VTK_SOURCE_DIR}/Wrapping/Tcl)

  add_test(NAME ${vtk-module}Tcl-${tfile}
    COMMAND ${VTK_TCL_EXE} ${_test_args})
endforeach()
