vtk_add_test_cxx(vtkCommonSystemCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestDirectory.cxx
  otherTimerLog.cxx
  )

if(NOT WIN32)
  vtk_add_test_cxx(vtkCommonSystemCxxTests tests
    NO_DATA NO_VALID NO_OUTPUT
    # command line process is not as trivial to test on Windows as on Unix systems
    TestCommandLineProcess.cxx
    )
endif()

vtk_test_cxx_executable(vtkCommonSystemCxxTests tests)
