SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN 
"
    int interactive = 0;
    for (int ii = 0; ii < ac; ++ii)
      {
      if ( strcmp(av[ii],\"-I\") == 0)
        {
        interactive = 1;
        break;
        }
      }
    vtkSmartPointer<vtkTestingObjectFactory> factory = vtkSmartPointer<vtkTestingObjectFactory>::New();
    if (!interactive)
      {
      vtkObjectFactory::RegisterFactory(factory);
      vtkTestingInteractor::TestName=vtkstd::string(cmakeGeneratedFunctionMapEntries[testToRun].name);
      vtkTestingInteractor::TempDirectory=vtkstd::string(\"${VTK_BINARY_DIR}/Testing/Temporary\");
      vtkTestingInteractor::BaselineDirectory=vtkstd::string(\"${VTK_DATA_ROOT}\") + vtkstd::string(\"/Baseline/\") + vtkstd::string(\"${KIT}\");
      vtkTestingInteractor::DataDirectory=vtkstd::string(\"${VTK_DATA_ROOT}\");
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

