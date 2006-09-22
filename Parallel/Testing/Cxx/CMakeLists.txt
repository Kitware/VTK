IF(VTK_USE_DISPLAY AND VTK_USE_RENDERING)
  ADD_EXECUTABLE(SocketServer SocketServer.cxx)
  ADD_EXECUTABLE(SocketClient SocketClient.cxx)
  TARGET_LINK_LIBRARIES(SocketClient vtkParallel)
  TARGET_LINK_LIBRARIES(SocketServer vtkParallel)

  ADD_EXECUTABLE(PrmMagnifyTest PrmMagnify.cxx)
  TARGET_LINK_LIBRARIES(PrmMagnifyTest vtkParallel)

  IF (VTK_DATA_ROOT)
    ADD_TEST(PrmMagnify-image 
      ${CXX_TEST_PATH}/PrmMagnifyTest
      -D ${VTK_DATA_ROOT}
      -T ${VTK_BINARY_DIR}/Testing/Temporary
      -V Baseline/Parallel/PrmMagnify.png
      )
  ENDIF(VTK_DATA_ROOT)

  ADD_EXECUTABLE(TestTemporalFractal TestTemporalFractal.cxx)
  TARGET_LINK_LIBRARIES(TestTemporalFractal vtkParallel vtkHybrid)
  IF (VTK_DATA_ROOT)
    ADD_TEST(TestTemporalFractal-image 
      ${CXX_TEST_PATH}/TestTemporalFractal
      -D ${VTK_DATA_ROOT}
      -T ${VTK_BINARY_DIR}/Testing/Temporary
      -V Baseline/Parallel/TestTemporalFractal.png
      )
  ENDIF(VTK_DATA_ROOT)

  ADD_EXECUTABLE(TestExodusTime TestExodusTime.cxx)
  TARGET_LINK_LIBRARIES(TestExodusTime vtkParallel vtkHybrid)

  IF (VTK_USE_MPI)
    ADD_EXECUTABLE(ParallelIsoTest ParallelIso.cxx)
    TARGET_LINK_LIBRARIES(ParallelIsoTest vtkVolumeRendering vtkParallel vtkHybrid vtkWidgets)

    ADD_EXECUTABLE(GenericCommunicator GenericCommunicator.cxx)
    TARGET_LINK_LIBRARIES(GenericCommunicator vtkParallel)

    ADD_EXECUTABLE(TestPStream TestPStream.cxx)
    TARGET_LINK_LIBRARIES(TestPStream vtkParallel)

    ADD_EXECUTABLE(MPIGroups MPIGroups.cxx)
    TARGET_LINK_LIBRARIES(MPIGroups vtkParallel)

    ADD_EXECUTABLE(DistributedData DistributedData.cxx)
    TARGET_LINK_LIBRARIES(DistributedData vtkParallel)

    ADD_EXECUTABLE(TransmitImageData TransmitImageData.cxx)
    TARGET_LINK_LIBRARIES(TransmitImageData vtkParallel)

    ADD_EXECUTABLE(TransmitRectilinearGrid TransmitRectilinearGrid.cxx)
    TARGET_LINK_LIBRARIES(TransmitRectilinearGrid vtkParallel)

    ADD_EXECUTABLE(TransmitStructuredGrid TransmitStructuredGrid.cxx)
    TARGET_LINK_LIBRARIES(TransmitStructuredGrid vtkParallel)

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
        IF (VTK_MPI_MAX_NUMPROCS EQUAL 4)
          ADD_TEST(MPIGroups-image 
            ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS} ${VTK_MPI_PREFLAGS} 
            ${CXX_TEST_PATH}/MPIGroups
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/MPIGroups.cxx.png
            ${VTK_MPI_POSTFLAGS})
        ENDIF (VTK_MPI_MAX_NUMPROCS EQUAL 4)

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
  IF (UNIX)
    IF (PYTHON_EXECUTABLE)
      IF (VTK_DATA_ROOT)
        ADD_TEST(Sockets-image ${PYTHON_EXECUTABLE}
          ${VTK_SOURCE_DIR}/Parallel/Testing/Cxx/TestSockets.py 
          ${VTK_EXECUTABLE_DIR}/SocketServer
          ${VTK_EXECUTABLE_DIR}/SocketClient
          ${VTK_DATA_ROOT}
          Baseline/Parallel/SocketClient.cxx.png
          ${VTK_BINARY_DIR}/Testing/Temporary)
      ENDIF (VTK_DATA_ROOT)
    ENDIF (PYTHON_EXECUTABLE)
  ENDIF (UNIX)
ENDIF(VTK_USE_DISPLAY AND VTK_USE_RENDERING)
