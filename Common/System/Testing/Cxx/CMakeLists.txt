vtk_add_test_cxx(vtkCommonSystemCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestCommandLineProcess.cxx
  TestDirectory.cxx
  otherTimerLog.cxx
  )

vtk_test_cxx_executable(vtkCommonSystemCxxTests tests)
