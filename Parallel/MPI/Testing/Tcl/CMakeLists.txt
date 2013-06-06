unset(tests)
set(tests
  TestPProbe
  )

foreach( tfile ${tests})
  ExternalData_add_test(VTKData
    NAME ${vtk-module}Tcl-${tfile} COMMAND
    ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
    $<TARGET_FILE:pvtk>
    ${vtkTestingRendering_SOURCE_DIR}/prtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_TEST_DATA_DIR}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V DATA{../Data/Baseline/${tfile}.png}
    -A ${VTK_SOURCE_DIR}/Wrapping/Tcl
    ${VTK_MPI_POSTFLAGS}
   )
endforeach()
