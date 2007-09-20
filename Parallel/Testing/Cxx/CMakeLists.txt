IF(VTK_USE_DISPLAY AND VTK_USE_RENDERING)
  SET(KIT Parallel)
  # add tests that do not require data
  SET(MyTests
    DummyController.cxx
    TestTemporalCacheTemporal.cxx
    TestTemporalCacheSimple.cxx
    )
  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests ${MyTests}
      PrmMagnify.cxx
      #TestExodusTime.cxx
      TestTemporalFractal.cxx
      )
  ENDIF (VTK_DATA_ROOT)    
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                         EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests} ExerciseMultiProcessController.cxx)
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO vtkParallel vtkHybrid)
  SET (TestsToRun ${Tests})
  REMOVE (TestsToRun ${KIT}CxxTests.cxx)

  # Add all the standard tests
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/${KIT}/${TName}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}) 
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test) 

  # The following tests are launched special because they use MPI or
  # have sockets.

  IF(HAVE_SOCKETS)
    ADD_EXECUTABLE(SocketServer
      SocketServer.cxx ExerciseMultiProcessController.cxx)
    ADD_EXECUTABLE(SocketClient
      SocketClient.cxx ExerciseMultiProcessController.cxx)
    TARGET_LINK_LIBRARIES(SocketClient vtkParallel)
    TARGET_LINK_LIBRARIES(SocketServer vtkParallel)
  ENDIF(HAVE_SOCKETS)

  IF (VTK_USE_MPI)
    ADD_EXECUTABLE(ParallelIsoTest ParallelIso.cxx)
    TARGET_LINK_LIBRARIES(ParallelIsoTest vtkVolumeRendering vtkParallel vtkHybrid vtkWidgets)

    ADD_EXECUTABLE(TestMPIController MPIController.cxx
      ExerciseMultiProcessController.cxx)
    TARGET_LINK_LIBRARIES(TestMPIController vtkParallel)

    ADD_EXECUTABLE(GenericCommunicator GenericCommunicator.cxx)
    TARGET_LINK_LIBRARIES(GenericCommunicator vtkParallel)

    ADD_EXECUTABLE(TestPStream TestPStream.cxx)
    TARGET_LINK_LIBRARIES(TestPStream vtkParallel)

    ADD_EXECUTABLE(DistributedData DistributedData.cxx)
    TARGET_LINK_LIBRARIES(DistributedData vtkParallel)

    ADD_EXECUTABLE(TransmitImageData TransmitImageData.cxx)
    TARGET_LINK_LIBRARIES(TransmitImageData vtkParallel)

    ADD_EXECUTABLE(TransmitRectilinearGrid TransmitRectilinearGrid.cxx)
    TARGET_LINK_LIBRARIES(TransmitRectilinearGrid vtkParallel)

    ADD_EXECUTABLE(TransmitStructuredGrid TransmitStructuredGrid.cxx)
    TARGET_LINK_LIBRARIES(TransmitStructuredGrid vtkParallel)

    IF (VTK_MPIRUN_EXE)
      ADD_TEST(MPIController
        ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS}
        ${CXX_TEST_PATH}/TestMPIController
        ${VTK_MPI_POSTFLAGS}
        )
    ENDIF (VTK_MPIRUN_EXE)

    # This definition looks wrong in multiple respects:
    # 1. It's called an image test, but it does not actually compare an image.
    # 2. It uses VTK_MPIRUN_EXE, but is not wrapped in a IF (VTK_MPIRUN_EXE)
    #    block.
    # 3. It is duplicated by the TestCommunicator test (when VTK_DATA_ROOT is
    #    not defined).
    # Since the functionality and more is tested by the MPIController test,
    # I vote we just get rid of it altogether.
    ADD_TEST(GenericCommunicator-image 
      ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} 
      ${CXX_TEST_PATH}/GenericCommunicator
      ${VTK_MPI_POSTFLAGS})

    #
    # Add tests, with the data
    #

    IF (VTK_DATA_ROOT)
      IF (VTK_MPIRUN_EXE)
        IF(VTK_MPI_MAX_NUMPROCS GREATER 1)
          ADD_TEST(ParallelIso-image 
            ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} 
            ${CXX_TEST_PATH}/ParallelIsoTest
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/ParallelIso.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TestPStream-image 
            ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} 
            ${CXX_TEST_PATH}/TestPStream
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TestPStream.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(DistributedData-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/DistributedData
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/DistributedData.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TransmitImageData-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/TransmitImageData
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TransmitImageData.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TransmitRectilinearGrid-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/TransmitRectilinearGrid
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TransmitRectilinearGrid.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TransmitStructuredGrid-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/TransmitStructuredGrid
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Tempor2ary
            -V Baseline/Parallel/TransmitStructuredGrid.cxx.png
            ${VTK_MPI_POSTFLAGS})
        ENDIF(VTK_MPI_MAX_NUMPROCS GREATER 1)

      ENDIF (VTK_MPIRUN_EXE)
      #
      # If we do not have the data, still run the tests that we can
      #
    ELSE (VTK_DATA_ROOT)
        IF (VTK_MPIRUN_EXE)
          ADD_TEST(TestCommunicator 
            ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} 
            ${CXX_TEST_PATH}/GenericCommunicator ${VTK_MPI_POSTFLAGS})
        ENDIF(VTK_MPIRUN_EXE)
    ENDIF (VTK_DATA_ROOT)
  ENDIF (VTK_USE_MPI)

  # For now this test is only available on Unix because
  # on Windows, python does not support forking/killing processes
  IF (UNIX  AND  PYTHON_EXECUTABLE  AND  VTK_DATA_ROOT  AND  HAVE_SOCKETS)
    ADD_TEST(Sockets-image ${PYTHON_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Parallel/Testing/Cxx/TestSockets.py 
      ${VTK_EXECUTABLE_DIR}/SocketServer
      ${VTK_EXECUTABLE_DIR}/SocketClient
      ${VTK_DATA_ROOT}
      Baseline/Parallel/SocketClient.cxx.png
      ${VTK_BINARY_DIR}/Testing/Temporary)
  ENDIF (UNIX  AND  PYTHON_EXECUTABLE  AND  VTK_DATA_ROOT  AND  HAVE_SOCKETS)
ENDIF(VTK_USE_DISPLAY AND VTK_USE_RENDERING)
