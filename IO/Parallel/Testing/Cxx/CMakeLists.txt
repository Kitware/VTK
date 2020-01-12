vtk_add_test_cxx(vtkIOParallelCxxTests tests
  TestPOpenFOAMReader.cxx
  TestBigEndianPlot3D.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOParallelCxxTests tests)
