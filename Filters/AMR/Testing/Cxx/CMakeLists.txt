vtk_add_test_cxx(vtkFiltersAMRCxxTests tests
  TestAMRAbortExecute.cxx,NO_VALID
  TestAMRContour.cxx,NO_VALID
  TestAMRInterface.cxx,NO_VALID
  TestAMRSliceFilterCellData.cxx
  TestAMRSliceFilterPointData.cxx
  TestAMRGhostLayerStripping.cxx,NO_VALID
  TestAMRBlanking.cxx,NO_VALID
  TestImageToAMR.cxx,NO_VALID
  TestAMRGhostZones.cxx,NO_VALID
  TestAMRRefinedGhostFlag.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkFiltersAMRCxxTests tests)
