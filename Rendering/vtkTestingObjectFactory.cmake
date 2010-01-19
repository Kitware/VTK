SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN 
"
    // Set defaults
    vtkTestingInteractor::ValidBaseline =
      std::string(\"${VTK_DATA_ROOT}\") +
      std::string(\"/Baseline/\") +
      std::string(\"${KIT}/\") +
      std::string(cmakeGeneratedFunctionMapEntries[testToRun].name) +
      std::string(\".png\");
    vtkTestingInteractor::TempDirectory =
      std::string(\"${VTK_BINARY_DIR}/Testing/Temporary\");
    vtkTestingInteractor::DataDirectory =
      std::string(\"${VTK_DATA_ROOT}\");

    int interactive = 0;
    for (int ii = 0; ii < ac; ++ii)
      {
      if ( strcmp(av[ii],\"-I\") == 0)
        {
        interactive = 1;
        continue;
        }
      if ( strcmp(av[ii],\"-V\") == 0)
        {
        vtkTestingInteractor::ValidBaseline = std::string(av[ii+1]);
        ++ii;
        continue;
        }
      if ( strcmp(av[ii],\"-T\") == 0)
        {
        vtkTestingInteractor::TempDirectory = std::string(av[ii+1]);
        ++ii;
        continue;
        }
      if ( strcmp(av[ii],\"-D\") == 0)
        {
        vtkTestingInteractor::DataDirectory = std::string(av[ii+1]);
        ++ii;
        continue;
        }
      }
    vtkSmartPointer<vtkTestingObjectFactory> factory = vtkSmartPointer<vtkTestingObjectFactory>::New();
    if (!interactive)
      {
      vtkObjectFactory::RegisterFactory(factory);
      }
"
)

SET(CMAKE_TESTDRIVER_AFTER_TESTMAIN
"    
   if (!interactive)
     {
     if (vtkTestingInteractor::TestReturnStatus != -1)
        {
        if( vtkTestingInteractor::TestReturnStatus != vtkTesting::PASSED)
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
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                       EXTRA_INCLUDE ${VTK_SOURCE_DIR}/Rendering/vtkTestingObjectFactory.h)
