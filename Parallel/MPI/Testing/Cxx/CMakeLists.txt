vtk_add_test_mpi(TestDistributedDataShadowMapPass.cxx)
vtk_add_test_mpi(TestNonBlockingCommunication.cxx)
vtk_add_test_mpi(GenericCommunicator.cxx)
vtk_add_test_mpi(MPIController.cxx ExerciseMultiProcessController.cxx)
vtk_add_test_mpi(TestProcess.cxx)

if (VTK_DATA_ROOT)
  vtk_add_test_mpi(TestPProbe.cxx VTK_DATA_ROOT)
endif()
