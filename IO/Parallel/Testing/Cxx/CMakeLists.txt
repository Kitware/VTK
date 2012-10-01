vtk_tests(TestWindBladeReader.cxx)

# Parallel tests
add_test_mpi(PSLACReaderLinear.cxx)
add_test_mpi(PSLACReaderQuadratic.cxx)
