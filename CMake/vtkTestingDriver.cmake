SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN
"
  vtksys::SystemInformation::SetStackTraceOnError(1);

  // turn on windows stack traces if applicable
  vtkWindowsTestUtilitiesSetupForTesting();

  // init logging.
  vtkLogger::Init(ac, av);
")

SET(CMAKE_TESTDRIVER_AFTER_TESTMAIN
"
#if defined(__EMSCRIPTEN__)
  // When asyncify is used, emscripten sets up the wasm module 
  // such that the `main` function always returns 1.
  // Explicitly post the test's exit code back to the server.
  if (emscripten_has_asyncify())
  {
    vtkEmscriptenTestUtilities::PostExitCode(result);
  }
#endif
")
