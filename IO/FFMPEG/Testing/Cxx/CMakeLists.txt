vtk_add_test_cxx(vtkIOFFMPEGCxxTests tests
  NO_VALID
  TestFFMPEGWriter.cxx
  )
vtk_test_cxx_executable(vtkIOFFMPEGCxxTests tests)
