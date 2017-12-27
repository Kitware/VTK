ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/TRUCHAS/,REGEX:.*}"
)

vtk_add_test_cxx(vtkIOTRUCHASCxxTests tests
  TestTRUCHASReader.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOTRUCHASCxxTests tests
  )
