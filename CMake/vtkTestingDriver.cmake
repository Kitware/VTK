SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN
"
  vtksys::SystemInformation::SetStackTraceOnError(1);

  // init logging.
  vtkLogger::Init(ac, av);
")

SET(CMAKE_TESTDRIVER_AFTER_TESTMAIN "")
