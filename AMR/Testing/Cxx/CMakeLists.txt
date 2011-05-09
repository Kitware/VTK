## Set the name of the KIT being tested
set(KIT AMR)

## List all test sources
set(MyTests
    TestFlashReader.cxx
    TestEnzoReader.cxx 
    )
    
## Create test source list
create_test_sourcelist(Tests ${KIT}CxxTests.cxx
    ${MyTests}    
    EXTRA_INCLUDE vtkTestDriver.h
    )
add_executable( ${KIT}CxxTests ${Tests})
target_link_libraries( ${KIT}CxxTests vtkFiltering vtkParallel )
    
set(TestsToRun ${MyTests})

## Add all executables
foreach(test ${TestToRun})
  get_filename_component(t ${test} NAME_WE)
  add_test(${t} ${CXX_TEST_PATH}/${KIT}CxxTests ${t})
end foreach(test)