set(MyTests
  TestBSPTree.cxx
  TestStreamTracer
  TestAMRInterpolatedVelocityField
  TestParticleTracers
  )

include(vtkTestingObjectFactory)
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun CxxTests.cxx)

# Add all the executables
foreach (test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)

  if(VTK_DATA_ROOT)
  add_test(NAME ${vtk-module}Cxx-${TName}
    COMMAND ${vtk-module}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V Baseline/Graphics/${TName}.png)
  else()
  add_test(NAME ${vtk-module}Cxx-${TName}
    COMMAND ${vtk-module}CxxTests ${TName}
      -T ${VTK_TEST_OUTPUT_DIR}
      )
  endif()
endforeach()
