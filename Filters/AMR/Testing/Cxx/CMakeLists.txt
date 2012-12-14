## List all test sources
set(MyTests
    TestAMRGhostLayerStripping.cxx
    TestAMRBlanking.cxx
    TestAMRIterator.cxx
    TestImageToAMR.cxx
    )

## Create test source list
create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
    ${MyTests}
    EXTRA_INCLUDE vtkTestDriver.h
    )
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

## Add all executables
foreach(test ${TestsToRun})
  get_filename_component(t ${test} NAME_WE)
  add_test(NAME ${vtk-module}Cxx-${t}
    COMMAND ${vtk-module}CxxTests ${t} -D ${VTK_DATA_ROOT})
  set_tests_properties(${vtk-module}Cxx-${t} PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
endforeach()
