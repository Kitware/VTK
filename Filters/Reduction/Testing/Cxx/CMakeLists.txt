vtk_add_test_cxx(vtkFiltersReductionCxxTests tests
)

set(implicit_no_data_tests
    TestToAffineArrayStrategy.cxx
    TestToConstantArrayStrategy.cxx
    TestToImplicitArrayFilter.cxx
    TestToImplicitRamerDouglasPeuckerStrategy.cxx
    TestToImplicitTypeErasureStrategy.cxx
  )

vtk_add_test_cxx(vtkFiltersReductionCxxTests no_data_tests
  NO_DATA NO_VALID NO_OUTPUT
  ${implicit_no_data_tests}
)

set(all_tests ${tests} ${no_data_tests})
vtk_test_cxx_executable(vtkFiltersReductionCxxTests all_tests)
