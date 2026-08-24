SET(_VTK_FLT_EXCEPTIONS "vtkFloatingPointExceptions::Enable()")
IF(${DISABLE_FLOATING_POINT_EXCEPTIONS})
  SET(_VTK_FLT_EXCEPTIONS "vtkFloatingPointExceptions::Disable()")
ENDIF()

SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN
"
    vtksys::SystemInformation::SetStackTraceOnError(1);
#ifndef NDEBUG
    ${_VTK_FLT_EXCEPTIONS};
#endif

    // Set defaults
    vtkTestingInteractor::SetValidBaseline(\"Use_-V_for_Baseline\");
    vtkTestingInteractor::SetTempDirectory(\"${_vtk_build_TEST_OUTPUT_DIRECTORY}\");
    vtkTestingInteractor::SetDataDirectory(\"Use_-D_for_Data\");

    int interactive = 0;
    for (int ii = 0; ii < ac; ++ii)
    {
      if (strcmp(av[ii], \"-I\") == 0)
      {
        interactive = 1;
        continue;
      }
      if (ii < ac-1 && strcmp(av[ii], \"-V\") == 0)
      {
        vtkTestingInteractor::SetValidBaseline(av[++ii]);
        continue;
      }
      if (ii < ac-1 && strcmp(av[ii], \"-T\") == 0)
      {
        vtkTestingInteractor::SetTempDirectory(av[++ii]);
        continue;
      }
      if (ii < ac-1 && strcmp(av[ii], \"-D\") == 0)
      {
        vtkTestingInteractor::SetDataDirectory(av[++ii]);
        continue;
      }
      if (ii < ac-1 && strcmp(av[ii], \"-E\") == 0)
      {
        double threshold;
        VTK_FROM_CHARS_IF_ERROR_RETURN(av[++ii], threshold, EXIT_FAILURE);
        vtkTestingInteractor::SetErrorThreshold(threshold);
        continue;
      }
      if (ii < ac-1 && strcmp(av[ii], \"-v\") == 0)
      {
        int verbosity = 0;
        VTK_FROM_CHARS_IF_ERROR_RETURN(av[++ii], verbosity, EXIT_FAILURE);
        vtkLogger::SetStderrVerbosity(static_cast<vtkLogger::Verbosity>(verbosity));
        continue;
      }
    }

    // init logging
    vtkLogger::Init(ac, av, nullptr);

    // init object factory preferences
    vtkObjectFactory::InitializePreferencesFromCommandLineArgs(ac, av);

    // turn on windows stack traces if applicable
    vtkWindowsTestUtilitiesSetupForTesting();

    vtkSmartPointer<vtkTestingObjectFactory> factory = vtkSmartPointer<vtkTestingObjectFactory>::New();
    if (!interactive)
    {
      // Disable any other overrides before registering our factory.
      vtkObjectFactoryCollection *collection = vtkObjectFactory::GetRegisteredFactories();
      collection->InitTraversal();
      vtkObjectFactory *f = collection->GetNextItem();
      while (f)
      {
        f->Disable(\"vtkRenderWindowInteractor\");
        f = collection->GetNextItem();
      }
      vtkObjectFactory::RegisterFactory(factory);
    }
"
)

SET(CMAKE_TESTDRIVER_AFTER_TESTMAIN
"
  if (result == VTK_SKIP_RETURN_CODE)
  {
    vtk::print(\"Unsupported runtime configuration: Test returned \"
           \"VTK_SKIP_RETURN_CODE. Skipping test.\\n\");
    return result;
  }

  if (!interactive)
  {
    if (vtkTestingInteractor::TestReturnStatus != -1)
    {
      if (vtkTestingInteractor::TestReturnStatus != vtkTesting::PASSED)
      {
        result = EXIT_FAILURE;
      }
      else
      {
        result = EXIT_SUCCESS;
      }
    }
    vtkObjectFactory::UnRegisterFactory(factory);
  }
"
)
