if(VTK_USE_LARGE_DATA)
  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/NetCDF/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/SLAC/ll-9cell-f523/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/SLAC/pic-example/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/SLAC/pillbox/,REGEX:.*}"
    )

  vtk_add_test_cxx(${vtk-module}CxxTests tests
    SLACMultipleModes.cxx
    SLACParticleReader.cxx
    SLACReaderLinear.cxx
    SLACReaderQuadratic.cxx
    TestMPASReader.cxx
    TestNetCDFCAMReader.cxx
    TestNetCDFPOPReader.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
