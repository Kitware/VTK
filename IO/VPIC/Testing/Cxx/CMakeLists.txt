if(VTK_USE_LARGE_DATA)
  # Tell ExternalData to fetch test input at build time.
  ExternalData_Expand_Arguments(VTKData _
    "DATA{${VTK_TEST_INPUT_DIR}/VPIC/global.vpc}"
    "DATA{${VTK_TEST_INPUT_DIR}/VPIC/fields/T.0/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/VPIC/fields/T.100/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/VPIC/hydro/T.0/,REGEX:.*}"
    "DATA{${VTK_TEST_INPUT_DIR}/VPIC/hydro/T.100/,REGEX:.*}"
    )

  vtk_add_test_cxx(${vtk-module}CxxTests tests
    TestVPICReader.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)
