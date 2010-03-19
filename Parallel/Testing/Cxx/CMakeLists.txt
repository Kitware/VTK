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

    # VS6 builds do not handle out-of-range double assignment to float
    # properly. Do not run TestMultiBlockExodusWrite on VS6 builds.
    #
    IF(NOT CMAKE_GENERATOR MATCHES "Visual Studio 6")
      SET(MyTests ${MyTests}
        TestMultiBlockExodusWrite.cxx
        )
    ENDIF(NOT CMAKE_GENERATOR MATCHES "Visual Studio 6")
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
    IF (MPI_LIBRARY) 
      SET(MPI_LIBRARIES ${MPI_LIBRARY})   
    ENDIF (MPI_LIBRARY)   
    IF (MPI_EXTRA_LIBRARY)   
      SET(MPI_LIBRARIES ${MPI_LIBRARIES} "${MPI_EXTRA_LIBRARY}")   
    ENDIF (MPI_EXTRA_LIBRARY) 
    ADD_EXECUTABLE(ParallelIsoTest ParallelIso.cxx)
    TARGET_LINK_LIBRARIES(ParallelIsoTest vtkVolumeRendering vtkParallel vtkHybrid vtkWidgets ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TestMPIController MPIController.cxx
      ExerciseMultiProcessController.cxx)
    TARGET_LINK_LIBRARIES(TestMPIController vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(GenericCommunicator GenericCommunicator.cxx)
    TARGET_LINK_LIBRARIES(GenericCommunicator vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TestPStream TestPStream.cxx)
    TARGET_LINK_LIBRARIES(TestPStream vtkParallel)

    ADD_EXECUTABLE(DistributedData DistributedData.cxx)
    TARGET_LINK_LIBRARIES(DistributedData vtkParallel)

    ADD_EXECUTABLE(DistributedDataRenderPass DistributedDataRenderPass.cxx)
    TARGET_LINK_LIBRARIES(DistributedDataRenderPass vtkParallel)

    ADD_EXECUTABLE(TransmitImageData TransmitImageData.cxx)
    TARGET_LINK_LIBRARIES(TransmitImageData vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TestProcess TestProcess.cxx)
    TARGET_LINK_LIBRARIES(TestProcess vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TransmitImageDataRenderPass TransmitImageDataRenderPass.cxx)
    TARGET_LINK_LIBRARIES(TransmitImageDataRenderPass vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TestPShadowMapPass TestPShadowMapPass.cxx)
    TARGET_LINK_LIBRARIES(TestPShadowMapPass vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TestDistributedDataShadowMapPass TestDistributedDataShadowMapPass.cxx)
    TARGET_LINK_LIBRARIES(TestDistributedDataShadowMapPass vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TestPCompositeZPass TestPCompositeZPass.cxx)
    TARGET_LINK_LIBRARIES(TestPCompositeZPass vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TestDistributedDataCompositeZPass TestDistributedDataCompositeZPass.cxx)
    TARGET_LINK_LIBRARIES(TestDistributedDataCompositeZPass vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TransmitRectilinearGrid TransmitRectilinearGrid.cxx)
    TARGET_LINK_LIBRARIES(TransmitRectilinearGrid vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(TransmitStructuredGrid TransmitStructuredGrid.cxx)
    TARGET_LINK_LIBRARIES(TransmitStructuredGrid vtkParallel ${MPI_LIBRARIES})

    ADD_EXECUTABLE(PSLACReaderLinear PSLACReaderLinear.cxx)
    TARGET_LINK_LIBRARIES(PSLACReaderLinear vtkParallel)

    ADD_EXECUTABLE(PSLACReaderQuadratic PSLACReaderQuadratic.cxx)
    TARGET_LINK_LIBRARIES(PSLACReaderQuadratic vtkParallel)

    IF (VTK_MPIRUN_EXE)
      ADD_TEST(MPIController
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS}
        ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestMPIController
        ${VTK_MPI_POSTFLAGS}
        )
      ADD_TEST(TestProcess
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestProcess
            ${VTK_MPI_POSTFLAGS})


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
      ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} 
      ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/GenericCommunicator
      ${VTK_MPI_POSTFLAGS})

    IF (VTK_USE_PARALLEL_BGL)
      # Add tests using the Parallel BGL
      ADD_EXECUTABLE(PBGLRandomGraph PBGLRandomGraph.cxx)
      TARGET_LINK_LIBRARIES(PBGLRandomGraph vtkParallel ${Boost_LIBRARIES} ${MPI_LIBRARIES})

      ADD_EXECUTABLE(PBGLNamedVertexGraph PBGLNamedVertexGraph.cxx)
      TARGET_LINK_LIBRARIES(PBGLNamedVertexGraph vtkParallel ${Boost_LIBRARIES} ${MPI_LIBRARIES})

      ADD_EXECUTABLE(TestPBGLGraphSQLReader TestPBGLGraphSQLReader.cxx)
      TARGET_LINK_LIBRARIES(TestPBGLGraphSQLReader vtkParallel ${Boost_LIBRARIES} ${MPI_LIBRARIES})

      ADD_EXECUTABLE(TestPBGLGraphSQLReaderFile TestPBGLGraphSQLReaderFile.cxx)
      TARGET_LINK_LIBRARIES(TestPBGLGraphSQLReaderFile vtkParallel ${Boost_LIBRARIES} ${MPI_LIBRARIES})
      
      ADD_TEST(PBGLRandomGraph
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS} 
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/PBGLRandomGraph
        ${VTK_MPI_POSTFLAGS})

      ADD_TEST(PBGLNamedVertexGraph
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS} 
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/PBGLNamedVertexGraph
        ${VTK_MPI_POSTFLAGS})

      ADD_TEST(TestPBGLGraphSQLReader
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPBGLGraphSQLReader
        ${VTK_MPI_POSTFLAGS})

      ADD_TEST(TestPBGLGraphSQLReaderFile
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPBGLGraphSQLReaderFile
        -D ${VTK_DATA_ROOT}
        ${VTK_MPI_POSTFLAGS})

      ADD_EXECUTABLE(TestPRandomGraphSource TestPRandomGraphSource.cxx)
      TARGET_LINK_LIBRARIES(TestPRandomGraphSource
        vtkParallel ${Boost_LIBRARIES} ${MPI_LIBRARIES})

      ADD_TEST(TestPRandomGraphSource
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPRandomGraphSource
        ${VTK_MPI_POSTFLAGS})

      ADD_EXECUTABLE(TestPRMATGraphSource TestPRMATGraphSource.cxx)
      TARGET_LINK_LIBRARIES(TestPRMATGraphSource
        vtkParallel ${Boost_LIBRARIES} ${MPI_LIBRARIES})
     
      ADD_TEST(TestPRMATGraphSource
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPRMATGraphSource
        ${VTK_MPI_POSTFLAGS})

      ADD_EXECUTABLE(TestPBGLCollapseGraph TestPBGLCollapseGraph.cxx)
      TARGET_LINK_LIBRARIES(TestPBGLCollapseGraph
        vtkParallel ${Boost_LIBRARIES} ${MPI_LIBRARIES})
      
      ADD_TEST(TestPBGLCollapseGraph
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPBGLCollapseGraph
        ${VTK_MPI_POSTFLAGS})

      ADD_EXECUTABLE(TestPBGLCollectGraph TestPBGLCollectGraph.cxx)
      TARGET_LINK_LIBRARIES(TestPBGLCollectGraph
        vtkParallel ${Boost_LIBRARIES} ${MPI_LIBRARIES})
      
      ADD_TEST(TestPBGLCollectGraph
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPBGLCollectGraph
        ${VTK_MPI_POSTFLAGS})

      ADD_EXECUTABLE(TestPBGLPipeline TestPBGLPipeline.cxx)
      TARGET_LINK_LIBRARIES(TestPBGLPipeline
        vtkParallel vtkViews ${Boost_LIBRARIES} ${MPI_LIBRARIES})
      
      ADD_TEST(TestPBGLPipeline
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPBGLPipeline
        ${VTK_MPI_POSTFLAGS})
        
      ADD_EXECUTABLE(TestPBGLPedigrees TestPBGLPedigrees.cxx)
      TARGET_LINK_LIBRARIES(TestPBGLPedigrees
        vtkParallel vtkViews ${Boost_LIBRARIES} ${MPI_LIBRARIES})
        
      ADD_TEST(TestPBGLPedigrees
        ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPBGLPedigrees
        ${VTK_MPI_POSTFLAGS})
        
      ADD_EXECUTABLE(TestPBGLEdgesPedigrees TestPBGLEdgesPedigrees.cxx)
      TARGET_LINK_LIBRARIES(TestPBGLEdgesPedigrees
        vtkParallel vtkViews ${Boost_LIBRARIES} ${MPI_LIBRARIES})
        
      ADD_TEST(TestPBGLEdgesPedigrees
        ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS} 
        ${CXX_TEST_PATH}/TestPBGLEdgesPedigrees
        ${VTK_MPI_POSTFLAGS})
        
    ENDIF (VTK_USE_PARALLEL_BGL)

    #
    # Add tests, with the data
    #

    IF (VTK_DATA_ROOT)
      IF (VTK_MPIRUN_EXE)
        IF(VTK_MPI_MAX_NUMPROCS GREATER 1)
          ADD_TEST(ParallelIso-image 
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} 
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/ParallelIsoTest
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/ParallelIso.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TestPStream-image 
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} 
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestPStream
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TestPStream.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(DistributedData-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/DistributedData
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/DistributedData.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(DistributedDataRenderPass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/DistributedDataRenderPass
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/DistributedDataRenderPass.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TransmitImageData-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TransmitImageData
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TransmitImageData.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TransmitImageDataRenderPass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TransmitImageDataRenderPass
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TransmitImageDataRenderPass.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TestPShadowMapPass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestPShadowMapPass
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TestPShadowMapPass.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TestDistributedDataShadowMapPass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestDistributedDataShadowMapPass
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TestDistributedDataShadowMapPass.cxx.png
            ${VTK_MPI_POSTFLAGS})

          ADD_TEST(TestPCompositeZPass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestPCompositeZPass
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TestPCompositeZPass.cxx.png
            ${VTK_MPI_POSTFLAGS})

          ADD_TEST(TestDistributedDataCompositeZPass-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TestDistributedDataCompositeZPass
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TestDistributedDataCompositeZPass.cxx.png
            ${VTK_MPI_POSTFLAGS})

          ADD_TEST(TransmitRectilinearGrid-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TransmitRectilinearGrid
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/TransmitRectilinearGrid.cxx.png
            ${VTK_MPI_POSTFLAGS})
          ADD_TEST(TransmitStructuredGrid-image
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
            ${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/TransmitStructuredGrid
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Tempor2ary
            -V Baseline/Parallel/TransmitStructuredGrid.cxx.png
            ${VTK_MPI_POSTFLAGS})
        ENDIF(VTK_MPI_MAX_NUMPROCS GREATER 1)

        IF(VTK_MPI_MAX_NUMPROCS GREATER 6)
          ADD_TEST(ParallelIso-7proc-image 
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 7 ${VTK_MPI_PREFLAGS} 
            ${CXX_TEST_PATH}/ParallelIsoTest
            -D ${VTK_DATA_ROOT}
            -T ${VTK_BINARY_DIR}/Testing/Temporary
            -V Baseline/Parallel/ParallelIso.7proc.png
            ${VTK_MPI_POSTFLAGS})
        ENDIF(VTK_MPI_MAX_NUMPROCS GREATER 6)

      ENDIF (VTK_MPIRUN_EXE)
      #
      # If we do not have the data, still run the tests that we can
      #
    ELSE (VTK_DATA_ROOT)
        IF (VTK_MPIRUN_EXE)
          ADD_TEST(TestCommunicator 
            ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} 
            ${CXX_TEST_PATH}/GenericCommunicator ${VTK_MPI_POSTFLAGS})
        ENDIF(VTK_MPIRUN_EXE)
    ENDIF (VTK_DATA_ROOT)
    IF (VTK_LARGE_DATA_ROOT)
      IF (VTK_MPIRUN_EXE)
        ADD_TEST(PSLACReaderLinear
          ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
          ${CXX_TEST_PATH}/PSLACReaderLinear
          -D ${VTK_LARGE_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V ${VTK_LARGE_DATA_ROOT}/Baseline/Parallel/PSLACReaderLinear.png
          ${VTK_MPI_POSTFLAGS})
        ADD_TEST(PSLACReaderQuadratic
          ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
          ${CXX_TEST_PATH}/PSLACReaderQuadratic
          -D ${VTK_LARGE_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V ${VTK_LARGE_DATA_ROOT}/Baseline/Parallel/PSLACReaderQuadratic.png
          ${VTK_MPI_POSTFLAGS})
      ENDIF (VTK_MPIRUN_EXE)
    ENDIF (VTK_LARGE_DATA_ROOT)
  ENDIF (VTK_USE_MPI)

  # For now this test is only available on Unix because
  # on Windows, python does not support forking/killing processes
  IF (UNIX  AND  PYTHON_EXECUTABLE  AND  VTK_DATA_ROOT  AND  HAVE_SOCKETS)
    SET(dir "${EXECUTABLE_OUTPUT_PATH}")

    # Allow this test to work from Xcode dashboards driven with ctest -C:
    #
    IF(CMAKE_CONFIGURATION_TYPES)
      IF(NOT "$ENV{CMAKE_CONFIG_TYPE}" STREQUAL "")
        SET(dir "${dir}/$ENV{CMAKE_CONFIG_TYPE}")
      ELSE(NOT "$ENV{CMAKE_CONFIG_TYPE}" STREQUAL "")
        SET(dir "${dir}/Release")
      ENDIF(NOT "$ENV{CMAKE_CONFIG_TYPE}" STREQUAL "")
    ENDIF(CMAKE_CONFIGURATION_TYPES)

    ADD_TEST(Sockets-image ${PYTHON_EXECUTABLE}
      ${VTK_SOURCE_DIR}/Parallel/Testing/Cxx/TestSockets.py 
      ${dir}/SocketServer
      ${dir}/SocketClient
      ${VTK_DATA_ROOT}
      Baseline/Parallel/SocketClient.cxx.png
      ${VTK_BINARY_DIR}/Testing/Temporary)
  ENDIF (UNIX  AND  PYTHON_EXECUTABLE  AND  VTK_DATA_ROOT  AND  HAVE_SOCKETS)
ENDIF(VTK_USE_DISPLAY AND VTK_USE_RENDERING)
