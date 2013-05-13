vtk_add_test_cxx(NO_DATA NO_VALID NO_OUTPUT
  TestAmoebaMinimizer.cxx
  TestMatrix3x3.cxx
  TestPolynomialSolversUnivariate.cxx
  TestQuaternion.cxx
  )

if(NOT VTK_LEGACY_REMOVE)
  vtk_add_test_cxx(NO_DATA NO_VALID NO_OUTPUT
    TestFastNumericConversion.cxx
    )
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests)
