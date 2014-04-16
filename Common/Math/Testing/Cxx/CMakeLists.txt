vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestAmoebaMinimizer.cxx
  TestMatrix3x3.cxx
  TestPolynomialSolversUnivariate.cxx
  TestQuaternion.cxx
  )
if(NOT VTK_LEGACY_REMOVE)
  vtk_add_test_cxx(${vtk-module}CxxTests extra_tests
    NO_DATA NO_VALID NO_OUTPUT
    TestFastNumericConversion.cxx
    )
  list(APPEND tests
    ${extra_tests})
endif()
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
