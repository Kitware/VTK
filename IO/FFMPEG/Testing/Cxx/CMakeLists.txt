vtk_add_test_cxx(vtkIOFFMPEGCxxTests tests
  TestFFMPEGVideoSource.cxx
  TestFFMPEGWriter.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkIOFFMPEGCxxTests tests)
