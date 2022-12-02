vtk_add_test_cxx(vtkFiltersReductionCxxTests tests
)

vtk_add_test_cxx(vtkFiltersReductionCxxTests no_data_tests
  NO_DATA NO_VALID NO_OUTPUT
)

set(all_tests ${tests} ${no_data_tests})
vtk_test_cxx_executable(vtkFiltersReductionCxxTests all_tests)
