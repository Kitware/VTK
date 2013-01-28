set(MyTests ${MyTests}
  TestLegacyCompositeDataReaderWriter.cxx
)

# Tests with data
if(VTK_DATA_ROOT)
  # Use the testing object factory, to reduce boilerplate code in tests.
  include(vtkTestingObjectFactory)
  vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

  set(TestsToRun ${Tests})
  list(REMOVE_ITEM TestsToRun CxxTests.cxx)

  # Add all the executables
  foreach(test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
      add_test(NAME ${vtk-module}-${TName}
        COMMAND ${vtk-module}CxxTests ${TName}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_TEST_OUTPUT_DIR}
          -V Baseline/Parallel/${TName}.png)
  endforeach ()
endif()
