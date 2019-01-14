vtk_add_test_cxx(vtkIOMotionFXCxxTests tests
  NO_OUTPUT
  TestMotionFXCFGReader2Gears.cxx
  TestMotionFXCFGReaderPlanetary.cxx
  TestMotionFXCFGReaderPositionFile.cxx
  )
vtk_test_cxx_executable(vtkIOMotionFXCxxTests tests)
