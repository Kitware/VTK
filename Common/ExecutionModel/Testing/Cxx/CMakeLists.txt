create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestCopyAttributeData.cxx
  TestImageDataToStructuredGrid.cxx
  TestSetInputDataObject.cxx
  TestTemporalSupport.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach (test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)

  if(VTK_DATA_ROOT)
  add_test(NAME ${TName}
    COMMAND ${vtk-module}CxxTests ${TName}
      -D ${VTK_DATA_ROOT}
      -T ${VTK_TEST_OUTPUT_DIR}
      -V Baseline/${vtk-module}/${TName}.png)
  else()
  add_test(NAME ${TName}
    COMMAND ${vtk-module}CxxTests ${TName}
      -T ${VTK_TEST_OUTPUT_DIR}
      )
  endif()
endforeach()
