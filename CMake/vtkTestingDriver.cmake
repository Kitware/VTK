SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN
"
  vtksys::SystemInformation::SetStackTraceOnError(1);

  // turn on windows stack traces if applicable
  vtkWindowsTestUtilitiesSetupForTesting();

  // init logging.
  vtkLogger::Init(ac, av);
")

SET(CMAKE_TESTDRIVER_AFTER_TESTMAIN "")
