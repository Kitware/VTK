set(openxr_tests "")

if (VTK_OPENXR_TESTING_ENABLE_RUNTIME_TESTS)
  list (APPEND openxrremoting_tests
    TestOpenXRRemotingInitialization.cxx,NO_DATA,NO_VALID)
endif ()

vtk_add_test_cxx(vtkRenderingOpenXRRemotingCxxTests tests ${openxrremoting_tests})
vtk_test_cxx_executable(vtkRenderingOpenXRRemotingCxxTests tests)
