## Set the name of the KIT being tested
SET(KIT AMR)

## List all test sources
SET(MyTests
    TestFlashReader.cxx
    TestEnzoReader.cxx 
    )
    
## Create test source list
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
    ${MyTests}    
    EXTRA_INCLUDE vtkTestDriver.h
    )
ADD_EXECUTABLE( ${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES( ${KIT}CxxTests vtkAMR vtkFiltering vtkParallel )
    
## Add all executables
FOREACH(test ${MyTests})
  GET_FILENAME_COMPONENT(t ${test} NAME_WE)
  ADD_TEST(${t} ${CXX_TEST_PATH}/${KIT}CxxTests ${t} -D ${VTK_DATA_ROOT})
ENDFOREACH(test)