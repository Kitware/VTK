include(vtkMPI)

vtk_add_test_mpi(TestRandomPContingencyStatisticsMPI.cxx)
vtk_add_test_mpi(TestRandomPKMeansStatisticsMPI.cxx)
vtk_add_test_mpi(TestRandomPMomentStatisticsMPI.cxx)
vtk_add_test_mpi(TestRandomPOrderStatisticsMPI.cxx)

vtk_mpi_link(TestRandomPContingencyStatisticsMPI)
vtk_mpi_link(TestRandomPKMeansStatisticsMPI)
vtk_mpi_link(TestRandomPMomentStatisticsMPI)
vtk_mpi_link(TestRandomPOrderStatisticsMPI)

# # -----------------------------------------------------------------------------
# # The following file was being compiled but never added as a testin older
# # vtk. Why is that so?
# vtk_add_test_mpi(TestRealDataPDescriptiveStatisticsMPI)
