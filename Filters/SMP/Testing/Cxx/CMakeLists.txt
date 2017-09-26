vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestSMPContour.cxx
  )
if (NOT VTK_LEGACY_REMOVE)
    vtk_add_test_cxx(${vtk-module}CxxTests tests
      NO_VALID
      TestThreadedSynchronizedTemplates3D.cxx
      TestThreadedSynchronizedTemplatesCutter3D.cxx
      TestSMPTransform.cxx
      TestSMPWarp.cxx
      )
endif()
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
