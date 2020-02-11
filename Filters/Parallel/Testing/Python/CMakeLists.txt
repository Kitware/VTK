vtk_add_test_python(RectOutline.py)
vtk_add_test_python(TestCutMaterial.py)
vtk_add_test_python(TestExtractCTHPart.py)
vtk_add_test_python(TestExtractCTHPartSolid.py)
vtk_add_test_python(TestExtrudePiece.py)
vtk_add_test_python(TestImageStreamer.py)
vtk_add_test_python(TestIntegrateAttributes.py NO_OUTPUT NO_VALID NO_RT)
vtk_add_test_python(TestRemoveGhosts.py NO_DATA NO_OUTPUT NO_VALID NO_RT)
vtk_add_test_python(TestResamplingInterpolator.py)

if(TARGET pvtkpython)
  add_test(NAME vtkFiltersParallelPython-testTransmit
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 4 ${MPIEXEC_PREFLAGS}
      $<TARGET_FILE:pvtkpython> ${CMAKE_CURRENT_SOURCE_DIR}/testTransmit.py
      ${VTK_MPI_POSTFLAGS})
endif()
