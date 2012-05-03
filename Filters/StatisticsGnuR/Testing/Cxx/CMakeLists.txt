# Indicate that we want to compile the R version of the test
add_definitions(-DUSE_GNU_R)

# These tests are in Filters/Statistics but perform R testnig when USE_GNU_R is defined
set(StatsTests TestCorrelativeStatistics.cxx
  TestContingencyStatistics.cxx
  TestPCAStatistics.cxx
  TestDescriptiveStatistics.cxx
  )

set(RTests TestRCalculatorFilter.cxx
  TestRInterface.cxx
  TestRRandomTableSource.cxx
  )

create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  ${StatsTests}
  ${RTests}
  EXTRA_INCLUDE vtkTestDriver.h
)

# remove the source file and give full path ( the source lives in Filters/Statistics )
foreach(_test ${StatsTests})
  list(REMOVE_ITEM Tests ${_test})
  list(APPEND Tests ${vtkFiltersStatistics_SOURCE_DIR}/Testing/Cxx/${_test})
endforeach()

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

# Add all the executables
foreach(test ${StatsTests} ${RTests})
  get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
endforeach()
