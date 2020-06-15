vtk_add_test_cxx(vtkIOIossCxxTests tests
  TestIossExodusRestarts.cxx,NO_VALID
  TestIossExodus.cxx,NO_VALID
  TestIossUnsupported.cxx,NO_VALID
  TestIossAttributes.cxx,NO_VALID
  TestIossNoElementBlocks.cxx,NO_VALID
  TestIossFilePatternMatching.cxx,NO_VALID
  TestIossTri6.cxx)

if (VTK_USE_LARGE_DATA)
  vtk_add_test_cxx(vtkIOIossCxxTests tests
    TestIossSuperelements.cxx,NO_VALID)
endif()

if (TARGET VTK::FiltersParallel AND TARGET VTK::RenderingParallel AND TARGET VTK::mpi)
  set (vtkIOIossCxxTests_NUMPROCS 4)
  vtk_add_test_mpi(vtkIOIossCxxTests tests
    TestIossExodusParitionedFiles.cxx,TESTING_DATA
    TestIossExodusRestarts.cxx,NO_VALID,TESTING_DATA)
endif()

vtk_test_cxx_executable(vtkIOIossCxxTests tests)
