IF (VTK_USE_HYBRID)
  ADD_EXECUTABLE(ParallelIsoTest ParallelIso.cxx)
  TARGET_LINK_LIBRARIES(ParallelIsoTest vtkParallel vtkHybrid)
ENDIF (VTK_USE_HYBRID)

ADD_EXECUTABLE(SocketServer SocketServer.cxx)
ADD_EXECUTABLE(SocketClient SocketClient.cxx)

IF (VTK_HAS_THREADS OR VTK_USE_MPI)
 ADD_EXECUTABLE(GenericCommunicator GenericCommunicator.cxx)
 TARGET_LINK_LIBRARIES(GenericCommunicator vtkParallel)
ENDIF (VTK_HAS_THREADS OR VTK_USE_MPI)

IF (VTK_HAS_THREADS)
 ADD_EXECUTABLE(ThreadedCommunicator ThreadedCommunicator.cxx)
 TARGET_LINK_LIBRARIES(ThreadedCommunicator vtkParallel)
ENDIF (VTK_HAS_THREADS)

TARGET_LINK_LIBRARIES(SocketServer vtkParallel)
TARGET_LINK_LIBRARIES(SocketClient vtkParallel)

IF (VTK_USE_MPI)
  ADD_EXECUTABLE(MPIGroups MPIGroups.cxx)
  TARGET_LINK_LIBRARIES(MPIGroups vtkParallel)
ENDIF (VTK_USE_MPI)

#
# Add tests, with the data
#

IF (VTK_DATA_ROOT)
  IF (VTK_USE_HYBRID)
    IF (VTK_HAS_THREADS)
      ADD_TEST(ThreadedCommunicator-image 
               ${CXX_TEST_PATH}/ThreadedCommunicator
               -D ${VTK_DATA_ROOT}
               -V Baseline/Parallel/ThreadedCommunicator.cxx.png)
    ENDIF (VTK_HAS_THREADS)

    IF (VTK_USE_MPI)
     IF (UNIX)
      IF (MPIRUN)
        ADD_TEST(ParallelIsoMPI-image ${MPIRUN} ${MPI_PREFLAGS}
                 ${CXX_TEST_PATH}/ParallelIsoTest
                 -D ${VTK_DATA_ROOT}
                 -V Baseline/Parallel/ParallelIso.cxx.png
                 ${MPI_POSTFLAGS})
        ADD_TEST(TestCommunicatorMPI-image ${MPIRUN} ${MPI_PREFLAGS}
                 ${CXX_TEST_PATH}/GenericCommunicator
                 -D ${VTK_DATA_ROOT}
                 -V Baseline/Parallel/GenericCommunicator.cxx.png
                 ${MPI_POSTFLAGS})
        IF (MPI_MAX_NUMPROCS)
          IF (MPI_MAX_NUMPROCS MATCHES "[1-9]*(0|[4-9])$")
           ADD_TEST(MPIGroups-image ${MPIRUN} -np ${MPI_MAX_NUMPROCS}
                   ${CXX_TEST_PATH}/MPIGroups
                   -D ${VTK_DATA_ROOT}
                   -V Baseline/Parallel/MPIGroups.cxx.png
                   ${MPI_POSTFLAGS})
          ENDIF (MPI_MAX_NUMPROCS MATCHES "[1-9]*(0|[4-9])$")
        ENDIF (MPI_MAX_NUMPROCS)
      ENDIF(MPIRUN)
     ENDIF (UNIX)
    ELSE (VTK_USE_MPI)
      IF (VTK_HAS_THREADS)
        ADD_TEST(ParallelIso-image ${CXX_TEST_PATH}/ParallelIsoTest
                 -D ${VTK_DATA_ROOT}
                 -V Baseline/Parallel/ParallelIso.cxx.png)
        ADD_TEST(TestCommunicator-image ${CXX_TEST_PATH}/GenericCommunicator
                 -D ${VTK_DATA_ROOT}
                 -V Baseline/Parallel/GenericCommunicator.cxx.png)
      ENDIF (VTK_HAS_THREADS)
    ENDIF (VTK_USE_MPI)
  ENDIF (VTK_USE_HYBRID)
#
# If we do not have the data, still run the tests that we can
#
ELSE (VTK_DATA_ROOT)
  IF (VTK_USE_HYBRID)
    IF (VTK_HAS_THREADS)
      ADD_TEST(ThreadedCommunicator ${CXX_TEST_PATH}/ThreadedCommunicator)
    ENDIF (VTK_HAS_THREADS)
    IF (VTK_USE_MPI)
     IF(UNIX)
      IF (MPIRUN)
        ADD_TEST(ParallelIsoMPI ${MPIRUN} ${MPI_PREFLAGS}
                 ${CXX_TEST_PATH}/ParallelIsoTest ${MPI_POSTFLAGS})
        ADD_TEST(TestCommunicatorMPI ${MPIRUN} ${MPI_PREFLAGS}
                 ${CXX_TEST_PATH}/GenericCommunicator ${MPI_POSTFLAGS})
      ENDIF(MPIRUN)
     ENDIF(UNIX)
    ELSE (VTK_USE_MPI)
      IF (VTK_HAS_THREADS)
        ADD_TEST(ParallelIso ${CXX_TEST_PATH}/ParallelIsoTest)
        ADD_TEST(TestCommunicator ${CXX_TEST_PATH}/GenericCommunicator)
      ENDIF (VTK_HAS_THREADS)
    ENDIF (VTK_USE_MPI)
  ENDIF (VTK_USE_HYBRID)
ENDIF (VTK_DATA_ROOT)

IF (EXECUTABLE_OUTPUT_PATH)
  SET (PARALLEL_EXEC_PATH ${EXECUTABLE_OUTPUT_PATH})
ELSE (EXECUTABLE_OUTPUT_PATH)
  SET (PARALLEL_EXEC_PATH ${VTK_BINARY_DIR}/Parallel/Testing/Cxx/${CMAKE_CFG_INTDIR})
ENDIF (EXECUTABLE_OUTPUT_PATH)

# For now this test is only available on Unix because
# on Windows, python does not support forking/killing processes
IF (UNIX)
 IF (PYTHON_EXECUTABLE)
  IF (VTK_DATA_ROOT)
    ADD_TEST(Sockets-image ${PYTHON_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Parallel/Testing/Cxx/TestSockets.py 
      ${PARALLEL_EXEC_PATH}/SocketServer
      ${PARALLEL_EXEC_PATH}/SocketClient
      ${VTK_DATA_ROOT}
      Baseline/Parallel/SocketClient.cxx.png)
  ENDIF (VTK_DATA_ROOT)
 ENDIF (PYTHON_EXECUTABLE)
ENDIF (UNIX)

