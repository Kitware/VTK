# Tests with data
ExternalData_Expand_Arguments(VTKData _
  "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/AMReX/MFIX-Exa/,RECURSE:,REGEX:.*}"
  "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/AMReX/Sample2D/,RECURSE:,REGEX:.*}"
  "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/AMReX/NodalMultiFab/,RECURSE:,REGEX:.*}"
  "DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/AMReX/NonZeroOrigin/,RECURSE:,REGEX:.*}"
  )

vtk_add_test_cxx(vtkIOAMRCxxTests tests
  NO_VALID NO_OUTPUT
  TestAMRFlashReader.cxx
  TestAMReXGridReaderNodalMultiFab.cxx
  TestAMReXParticlesReader.cxx
  TestAMReXGridReaderNonZeroOrigin.cxx
  TestEnzoReader.cxx
  TestVelodyneReader.cxx
  )
vtk_test_cxx_executable(vtkIOAMRCxxTests tests)
