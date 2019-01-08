vtk_add_test_cxx(vtkFiltersAMRCxxTests tests
  TestAMRSliceFilterCellData.cxx
  TestAMRSliceFilterPointData.cxx
  TestAMRGhostLayerStripping.cxx,NO_VALID
  TestAMRBlanking.cxx,NO_VALID
  TestAMRIterator.cxx,NO_VALID
  TestImageToAMR.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkFiltersAMRCxxTests tests)
