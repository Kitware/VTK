if(VTK_DATA_ROOT)
  set (NEEDS_DATA
    TestBiomTableReader.cxx
    TestNewickTreeReader.cxx
    TestMultiNewickTreeReader.cxx
    TestTulipReaderProperties.cxx
  )
endif()
create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  ${NEEDS_DATA}
  # TestTulipReader.cxx           # todo (unsatisfied deps)
  # TestTulipReaderClusters.cxx   # todo (unsatisfied deps)

  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Cxx-${TName}
            COMMAND ${vtk-module}CxxTests ${TName}
                                          -D ${VTK_DATA_ROOT}
                                          -T ${VTK_TEST_OUTPUT_DIR}
                                          -V Baseline/${vtk-module}/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName} COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
