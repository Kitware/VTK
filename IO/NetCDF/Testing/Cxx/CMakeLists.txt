vtk_add_test_cxx(vtkIONetCDFCxxTests tests
  SLACMultipleModes.cxx
  SLACParticleReader.cxx
  SLACReaderLinear.cxx
  SLACReaderQuadratic.cxx
  TestMPASReader.cxx
  TestNetCDFCAMReader.cxx
  TestNetCDFPOPReader.cxx
  )

vtk_test_cxx_executable(vtkIONetCDFCxxTests tests)
