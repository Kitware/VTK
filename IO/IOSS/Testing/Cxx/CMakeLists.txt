vtk_add_test_cxx(vtkIOIOSSCxxTests tests
  TestIOSSAssemblies.cxx,NO_VALID
  TestIOSSCGNS.cxx
  TestIOSSApplyDisplacementsCGNS.cxx
  TestIOSSExodusRestarts.cxx,NO_VALID
  TestIOSSExodus.cxx,NO_VALID
  TestIOSSUnsupported.cxx,NO_VALID
  TestIOSSAttributes.cxx,NO_VALID
  TestIOSSNoElementBlocks.cxx,NO_VALID
  TestIOSSFilePatternMatching.cxx,NO_VALID
  TestIOSSTri6.cxx)

if (VTK_USE_LARGE_DATA)
  vtk_add_test_cxx(vtkIOIOSSCxxTests tests
    TestIOSSSuperelements.cxx,NO_VALID)
endif()

if (TARGET VTK::FiltersParallel AND TARGET VTK::RenderingParallel AND TARGET VTK::mpi)
  set (vtkIOIOSSCxxTests_NUMPROCS 4)
  vtk_add_test_mpi(vtkIOIOSSCxxTests tests
    TestIOSSExodusParitionedFiles.cxx,TESTING_DATA
    TestIOSSExodusRestarts.cxx,NO_VALID,TESTING_DATA)
endif()

vtk_test_cxx_executable(vtkIOIOSSCxxTests tests)
