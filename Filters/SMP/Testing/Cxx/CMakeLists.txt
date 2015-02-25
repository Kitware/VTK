vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestSMPContour.cxx
  TestThreadedSynchronizedTemplates3D.cxx
  TestThreadedSynchronizedTemplatesCutter3D.cxx
  TestSMPTransform.cxx
  TestSMPWarp.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
