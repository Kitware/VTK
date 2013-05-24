if(VTK_USE_LARGE_DATA)
  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/LSDyna/hemi.draw/,REGEX:.*}"
    )

  vtk_add_test_mpi(PLSDynaReader.cxx TESTING_DATA)
endif()
