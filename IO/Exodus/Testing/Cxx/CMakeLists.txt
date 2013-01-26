# VS6 builds do not handle out-of-range double assignment to float
# properly. Do not run TestMultiBlockExodusWrite on VS6 builds.
#
if(NOT CMAKE_GENERATOR MATCHES "Visual Studio 6")
  set(MyTests ${MyTests}
    TestMultiBlockExodusWrite.cxx
    )
endif()


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
