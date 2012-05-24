# Tests with test images in Baseline/Parallel
#
unset(tests)
if(VTK_DATA_ROOT)
  set(tests
    TestPProbe
  )
endif()

foreach( tfile ${tests})
  add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND
    ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
    $<TARGET_FILE:pvtk>
    ${vtkTestingRendering_SOURCE_DIR}/prtImageTest.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
    -D ${VTK_DATA_ROOT}
    -T ${VTK_TEST_OUTPUT_DIR}
    -V Baseline/Parallel/${tfile}.png
    -A ${VTK_SOURCE_DIR}/Wrapping/Tcl
    ${VTK_MPI_POSTFLAGS}
   )
endforeach()
