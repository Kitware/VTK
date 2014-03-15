if(VTK_USE_LARGE_DATA)
  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/NetCDF/,REGEX:.*}"
    )

  vtk_add_test_cxx(${vtk-module}CxxTests tests
    TestPNetCDFPOPReader.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
