# Tests with data
ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/AMReX/MFIX-Exa/,RECURSE:,REGEX:.*}"
  )

vtk_add_test_cxx(vtkIOAMRCxxTests tests
  NO_VALID NO_OUTPUT
  #    TestFlashReader.cxx  there is not data for this test
  TestAMReXParticlesReader.cxx
  TestEnzoReader.cxx
  )
vtk_test_cxx_executable(vtkIOAMRCxxTests tests)
