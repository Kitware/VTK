add_test_mpi(TestDistributedDataShadowMapPass.cxx)
add_test_mpi(TestNonBlockingCommunication.cxx)
add_test_mpi(GenericCommunicator.cxx)
add_test_mpi(MPIController.cxx ExerciseMultiProcessController.cxx)
add_test_mpi(TestProcess.cxx)

if (VTK_DATA_ROOT)
  add_test_mpi(TestPProbe.cxx DATADIR ${VTK_DATA_ROOT})
endif()
