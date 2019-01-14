vtk_add_test_cxx(vtkIOParallelExodusCxxTests tests
  NO_VALID
  TestExodusImplicitArrays.cxx
  )
vtk_test_cxx_executable(vtkIOParallelExodusCxxTests tests)
