create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestAutoCorrelativeStatistics.cxx
  TestCorrelativeStatistics.cxx
  TestContingencyStatistics.cxx
  TestDescriptiveStatistics.cxx
  TestKMeansStatistics.cxx
  TestMultiCorrelativeStatistics.cxx
  TestOrderStatistics.cxx
  TestPCAStatistics.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${vtk-module}Cxx-${TName}
      COMMAND ${vtk-module}CxxTests ${TName})
endforeach()
