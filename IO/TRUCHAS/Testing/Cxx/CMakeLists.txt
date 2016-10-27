ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/TRUCHAS/,REGEX:.*}"
)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestTRUCHASReader.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  )
