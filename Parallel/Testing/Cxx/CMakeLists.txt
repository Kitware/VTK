LINK_LIBRARIES (
vtkParallel
vtkRendering
vtkIO
vtkGraphics
vtkImaging
vtkFiltering 
vtkCommon 
)

LINK_LIBRARIES( png zlib )

IF (VTK_USE_HYBRID)
    ADD_EXECUTABLE(ParallelIsoTest ParallelIso)
    TARGET_LINK_LIBRARIES(ParallelIsoTest  vtkHybrid)
ENDIF (VTK_USE_HYBRID)

ADD_EXECUTABLE(SocketServer SocketServer)
ADD_EXECUTABLE(SocketClient SocketClient)
ADD_EXECUTABLE(GenericCommunicator GenericCommunicator)
ADD_EXECUTABLE(ThreadedCommunicator ThreadedCommunicator)
ADD_EXECUTABLE(MPIGroups MPIGroups)
 
LINK_LIBRARIES ( ${OPENGL_LIBRARY} )
IF (VTK_MANGLE_MESA)
  LINK_LIBRARIES( ${MESA_LIBRARY})
  IF ( MESA_OS_LIBRARY )
    LINK_LIBRARIES(${MESA_OS_LIBRARY})
  ENDIF ( MESA_OS_LIBRARY )
ENDIF (VTK_MANGLE_MESA)

IF (CMAKE_HAS_X)
  LINK_LIBRARIES(-lXt)
  LINK_LIBRARIES(${CMAKE_X_LIBS})
ENDIF (CMAKE_HAS_X)

IF (VTK_USE_MPI)
  IF (MPI_LIBRARY)
      LINK_LIBRARIES (${MPI_LIBRARY})
    ENDIF (MPI_LIBRARY)
    IF (MPI_INCLUDE_PATH) 
      INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  ENDIF(MPI_INCLUDE_PATH)
ENDIF (VTK_USE_MPI)

#
# Add tests, with the data
#

IF (VTK_DATA_ROOT)
  IF (VTK_USE_HYBRID)
    ADD_TEST(ThreadedCommunicator-image 
             ${CXX_TEST_PATH}/ThreadedCommunicator
             -D ${VTK_DATA_ROOT}
             -V Baseline/Parallel/ThreadedCommunicator.cxx.png)

    IF (VTK_USE_MPI)
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
      ENDIF(MPIRUN)
    ELSE (VTK_USE_MPI)
      IF (CMAKE_USE_PTHREADS OR CMAKE_USE_SPROC)
        ADD_TEST(ParallelIso-image ${CXX_TEST_PATH}/ParallelIsoTest
                 -D ${VTK_DATA_ROOT}
                 -V Baseline/Parallel/ParallelIsoTest.png)
        ADD_TEST(TestCommunicator-image ${CXX_TEST_PATH}/GenericCommunicator
                 -D ${VTK_DATA_ROOT}
                 -V Baseline/Parallel/GenericCommunicator.cxx.png)
      ENDIF (CMAKE_USE_PTHREADS OR CMAKE_USE_SPROC)
    ENDIF (VTK_USE_MPI)
  ENDIF (VTK_USE_HYBRID)
#
# If we do not have the data, still run the tests that we can
#
ELSE (VTK_DATA_ROOT)
  IF (VTK_USE_HYBRID)
    ADD_TEST(ThreadedCommunicator-image 
             ${CXX_TEST_PATH}/ThreadedCommunicator)
    IF (VTK_USE_MPI)
      IF (MPIRUN)
        ADD_TEST(ParallelIsoMPI ${MPIRUN} ${MPI_PREFLAGS}
                 ${CXX_TEST_PATH}/ParallelIsoTest ${MPI_POSTFLAGS})
        ADD_TEST(TestCommunicatorMPI ${MPIRUN} ${MPI_PREFLAGS}
                 ${CXX_TEST_PATH}/GenericCommunicator ${MPI_POSTFLAGS})
      ENDIF(MPIRUN)
    ELSE (VTK_USE_MPI)
      ADD_TEST(ParallelIso ${CXX_TEST_PATH}/ParallelIsoTest)
      ADD_TEST(TestCommunicator ${CXX_TEST_PATH}/GenericCommunicator)
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

