SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN 
"
    vtkTestingObjectFactory* factory = vtkTestingObjectFactory::New();
    vtkObjectFactory::RegisterFactory(factory);
    factory->Delete();
    vtkTestingInteractor::TestName=vtkstd::string(cmakeGeneratedFunctionMapEntries[testToRun].name);;
    vtkTestingInteractor::TempDirectory=vtkstd::string(\"${VTK_BINARY_DIR}/Testing/Temporary\");
    vtkTestingInteractor::BaselineDirectory=vtkstd::string(\"${VTK_DATA_ROOT}\") + vtkstd::string(\"/Baseline/\") + vtkstd::string(\"${KIT}\");

"
)

SET(CMAKE_TESTDRIVER_AFTER_TESTMAIN
"    
   if (vtkTestingInteractor::TestReturnStatus != -1)
      {
      if( vtkTestingInteractor::TestReturnStatus != 1)
        {
        result = EXIT_FAILURE;
        }
      else
        {
        result = EXIT_SUCCESS;
        }
      vtkObjectFactory::UnRegisterFactory(factory);
      }
"
)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                       EXTRA_INCLUDE ${VTK_SOURCE_DIR}/Examples/vtkTestingObjectFactory.h)

