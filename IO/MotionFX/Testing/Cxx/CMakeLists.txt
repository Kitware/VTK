# Tests with data
ExternalData_Expand_Arguments(VTKData _
  "DATA{${VTK_TEST_INPUT_DIR}/MotionFX/,RECURSE:,REGEX:.*}"
  )

vtk_add_test_cxx(vtkIOMotionFXCxxTests tests
  NO_OUTPUT
  TestMotionFXCFGReader2Gears.cxx
  TestMotionFXCFGReaderPlanetary.cxx
  TestMotionFXCFGReaderPositionFile.cxx
  )
vtk_test_cxx_executable(vtkIOMotionFXCxxTests tests)
