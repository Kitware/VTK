find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  # BoostArrayLogWeighting.cxx    # todo (unsatisfied deps)
  BoostArrayRandomSparseArraySource.cxx
  TestBoostAdapter.cxx
  TestBoostAlgorithms.cxx
  TestBoostBetweennessClustering.cxx
  # TestBoostBrandesCentrality.cxx # todo (unsatisfied deps)
  TestBoostDividedEdgeBundling.cxx
  TestBoostExtractLargestComponent.cxx
  TestBoostSplitTableField.cxx

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
        -V Baseline/Infovis/${TName}.png)
  else()
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
  endif()
endforeach()
