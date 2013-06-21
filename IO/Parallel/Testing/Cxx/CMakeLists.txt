if(VTK_USE_LARGE_DATA)
  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/SLAC/ll-9cell-f523/,REGEX:.*}"
    )

  vtk_add_test_mpi(PSLACReaderLinear.cxx TESTING_DATA)
  vtk_add_test_mpi(PSLACReaderQuadratic.cxx TESTING_DATA)
endif()
