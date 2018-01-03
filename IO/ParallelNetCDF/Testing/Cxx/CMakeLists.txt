vtk_add_test_cxx(vtkIOParallelNetCDFCxxTests tests
  TestPNetCDFPOPReader.cxx
  )

vtk_test_cxx_executable(vtkIOParallelNetCDFCxxTests tests)
