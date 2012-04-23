IF(VTK_USE_BOOST)
  include_directories(${Boost_INCLUDE_DIRS})
ENDIF(VTK_USE_BOOST)

# if we have rendering add the following tests
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  SET(KIT Infovis)
  # add tests that do not require data
  SET(MyTests
    TestArcEdges
    TestAssignCoordinates
    TestCirclePackLayoutStrategy
    TestCollapseVerticesByArray
    TestContingencyStatistics
    TestConvexHull2D
    TestCorrelativeStatistics
    TestCosmicTreeLayoutStrategy
    TestDataObjectToTable
    TestDescriptiveStatistics
    TestExtractSelectedGraph
    TestGraph
    TestGraphAttributes
    TestGraphAlgorithms
    TestGraphHierarchicalBundle
    TestGraphLayoutStrategy
    TestGroupLeafVertices
    TestKCoreDecomposition
    TestKdTreeBoxSelection
    TestKMeansStatistics
    TestMergeGraphs
    TestMultiCorrelativeStatistics
    TestOrderStatistics
    TestPassArrays
    TestPassThrough
    TestPCAStatistics
    TestPruneTreeFilter
    TestRandomGraphSource
    TestRemoveIsolatedVertices
    TestSimple3DCirclesStrategy
    TestStreamGraph
    TestStringToCategory
    TestTable
    TestTableSplitColumnComponents.cxx
    TestTreeMapLayoutStrategy
    TestThresholdTable
    TestVariantArray
    TestVariant
    )
  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests ${MyTests}
      TestChacoGraphReader
      TestDelimitedTextReader
      TestDIMACSGraphReader
      TestKCoreLayout
      TestMergeTables
      TestISIReader
      TestFixedWidthTextReader
      TestRISReader
      TestStringToNumeric
      TestTulipReader
      TestTulipReaderClusters
      TestTulipReaderProperties
      )
  ENDIF (VTK_DATA_ROOT)

  IF (VTK_USE_BOOST)
    # add boost tests
    SET(MyTests ${MyTests}
      TestBoostAdapter
      TestBoostAlgorithms
      TestBoostExtractLargestComponent.cxx
      TestBoostBetweennessClustering.cxx
      )

     IF (VTK_USE_VIEWS)
       SET(MyTests ${MyTests}
         TestBoostBrandesCentrality
         TestTableToGraph
       )
    ENDIF (VTK_USE_VIEWS)


    IF (VTK_USE_PARALLEL_BGL)
      # The variant-serialization test is kept separate, because it
      # needs to link against the serialization library.
      ADD_EXECUTABLE(TestVariantSerialization TestVariantSerialization.cxx)
      TARGET_LINK_LIBRARIES(TestVariantSerialization vtkInfovis ${Boost_SERIALIZATION_LIBRARY})
      ADD_TEST(TestVariantSerialization ${CXX_TEST_PATH}/TestVariantSerialization)
    ENDIF (VTK_USE_PARALLEL_BGL)

    IF(VTK_DATA_ROOT)
      SET(MyTests ${MyTests}
        TestBoostSplitTableField
        )
    ENDIF(VTK_DATA_ROOT)

    IF (VTK_USE_PARALLEL_BGL)
      # Add tests using the Parallel BGL
      ADD_EXECUTABLE(TestPBGLAlgorithms TestPBGLAlgorithms.cxx)
      TARGET_LINK_LIBRARIES(TestPBGLAlgorithms vtkInfovis ${Boost_LIBRARIES} ${MPI_LIBRARIES})

      ADD_TEST(TestPBGLAlgorithms
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS}
        ${CXX_TEST_PATH}${CXX_TEST_CONFIG}/TestPBGLAlgorithms
        ${VTK_MPI_POSTFLAGS})
    ENDIF (VTK_USE_PARALLEL_BGL)
  ENDIF (VTK_USE_BOOST)

  IF(VTK_USE_PARALLEL AND VTK_USE_MPI)
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
    IF (MPI_LIBRARY)
      SET(MPI_LIBRARIES ${MPI_LIBRARY})
    ENDIF (MPI_LIBRARY)
    IF (MPI_EXTRA_LIBRARY)
      SET(MPI_LIBRARIES ${MPI_LIBRARIES} "${MPI_EXTRA_LIBRARY}")
    ENDIF (MPI_EXTRA_LIBRARY)

    # Add parallel tests only if PARALLEL and MPI are turned ON
    # These tests are kept separate, because they must be run with mpirun or mpiexec

    # TestRandomPMomentStatisticsMPI:
    # MPI test of parallel descriptive, correlative, multicorrelative and PCA statistics
    ADD_EXECUTABLE(TestRandomPMomentStatisticsMPI TestRandomPMomentStatisticsMPI.cxx)
    TARGET_LINK_LIBRARIES(TestRandomPMomentStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

    # TestRandomPContingencyStatisticsMPI:
    # MPI test of parallel contingency statistics
    ADD_EXECUTABLE(TestRandomPContingencyStatisticsMPI TestRandomPContingencyStatisticsMPI.cxx)
    TARGET_LINK_LIBRARIES(TestRandomPContingencyStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

    # TestRandomPOrderStatisticsMPI:
    # MPI test of parallel order statistics
    ADD_EXECUTABLE(TestRandomPOrderStatisticsMPI TestRandomPOrderStatisticsMPI.cxx)
    TARGET_LINK_LIBRARIES(TestRandomPOrderStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

    # TestRandomPKMeansStatisticsMPI:
    # MPI test of parallel KMeans statistics
    ADD_EXECUTABLE(TestRandomPKMeansStatisticsMPI TestRandomPKMeansStatisticsMPI.cxx)
    TARGET_LINK_LIBRARIES(TestRandomPKMeansStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

    # TestRealDataPDescriptiveStatisticsMPI:
    # MPI test of parallel descriptive statistics with real data
    ADD_EXECUTABLE(TestRealDataPDescriptiveStatisticsMPI TestRealDataPDescriptiveStatisticsMPI.cxx)
    TARGET_LINK_LIBRARIES(TestRealDataPDescriptiveStatisticsMPI vtkInfovis vtkParallel ${MPI_LIBRARIES})

    IF (VTK_MPIRUN_EXE)
      ADD_TEST(TestRandomPMomentStatisticsMPI
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS}
        ${CXX_TEST_PATH}${CXX_TEST_CONFIG}/TestRandomPMomentStatisticsMPI
        ${VTK_MPI_POSTFLAGS}
        )

      ADD_TEST(TestRandomPContingencyStatisticsMPI
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS}
        ${CXX_TEST_PATH}${CXX_TEST_CONFIG}/TestRandomPContingencyStatisticsMPI
        ${VTK_MPI_POSTFLAGS}
        )

      ADD_TEST(TestRandomPOrderStatisticsMPI
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS}
        ${CXX_TEST_PATH}${CXX_TEST_CONFIG}/TestRandomPOrderStatisticsMPI
        ${VTK_MPI_POSTFLAGS}
        )

      ADD_TEST(TestRandomPKMeansStatisticsMPI
        ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
        ${VTK_MPI_PREFLAGS}
        ${CXX_TEST_PATH}${CXX_TEST_CONFIG}/TestRandomPKMeansStatisticsMPI
        ${VTK_MPI_POSTFLAGS}
        )
    ENDIF (VTK_MPIRUN_EXE)

  ENDIF (VTK_USE_PARALLEL AND VTK_USE_MPI)

  SET(VTK_HAS_UINT64_ARRAY 0)
  IF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
    SET(VTK_HAS_UINT64_ARRAY 1)
  ELSE(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
    IF(VTK_SIZEOF_LONG MATCHES "^8$")
      SET(VTK_HAS_UINT64_ARRAY 1)
    ELSE(VTK_SIZEOF_LONG MATCHES "^8$")
      IF(VTK_SIZEOF___INT64 MATCHES "^8$")
        IF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
          SET(VTK_HAS_UINT64_ARRAY 1)
        ENDIF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
      ENDIF(VTK_SIZEOF___INT64 MATCHES "^8$")
    ENDIF(VTK_SIZEOF_LONG MATCHES "^8$")
  ENDIF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
  IF(VTK_HAS_UINT64_ARRAY)
    # add these tests only if vtkTypeUInt64Array exists
    SET(MyTests ${MyTests}
      TestSQLGraphReader
      TestTimePoint
      )
  ENDIF(VTK_HAS_UINT64_ARRAY)
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                         EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})

  SET (LIBS vtkInfovis vtkRendering vtkIO vtkCommon)
  IF(VTK_USE_VIEWS)
    SET(LIBS ${LIBS} vtkViews)
 ENDIF(VTK_USE_VIEWS)

  TARGET_LINK_LIBRARIES(${KIT}CxxTests ${LIBS})
  SET (TestsToRun ${Tests})
  LIST (REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

  #
  # Add all the executables
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      ADD_TEST(Infovis-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/${KIT}/${TName}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(Infovis-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test)
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
IF(VTK_USE_N_WAY_ARRAYS)

  CREATE_TEST_SOURCELIST(ArrayTests ArrayCxxTests.cxx
    ArrayAPIDenseCoordinates.cxx
    ArrayDiagonalMatrixSource.cxx
    ArrayDotProductSimilarity.cxx
    ArrayExtractFactoredArray.cxx
    ArrayMatricizeArray.cxx
    TestArrayNorm.cxx
    ArrayNormalizeMatrixVectors.cxx
    ArraySparseArrayToTable.cxx
    ArrayTableToSparseArray.cxx
    ArrayToTable.cxx
    ArrayTransposeMatrix.cxx
    TestTableToArray.cxx
    EXTRA_INCLUDE vtkTestDriver.h
    )

  ADD_EXECUTABLE(InfovisArrayCxxTests ${ArrayTests})
  TARGET_LINK_LIBRARIES(InfovisArrayCxxTests vtkInfovis)

  SET(ArrayTestsToRun ${ArrayTests})
  REMOVE(ArrayTestsToRun ArrayCxxTests.cxx)

  FOREACH(test ${ArrayTestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    ADD_TEST(Array-${TName} ${CXX_TEST_PATH}/InfovisArrayCxxTests ${TName})
  ENDFOREACH(test)

  IF(VTK_USE_BOOST)

    CREATE_TEST_SOURCELIST(BoostArrayTests BoostArrayCxxTests.cxx
      BoostArrayLogWeighting.cxx
      BoostArrayRandomSparseArraySource.cxx
      )

    ADD_EXECUTABLE(InfovisBoostArrayCxxTests ${BoostArrayTests})
    TARGET_LINK_LIBRARIES(InfovisBoostArrayCxxTests vtkInfovis)

    SET(BoostArrayTestsToRun ${BoostArrayTests})
    REMOVE(BoostArrayTestsToRun BoostArrayCxxTests.cxx)

    FOREACH(test ${BoostArrayTestsToRun})
      GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
      ADD_TEST(Boost-Array-${TName} ${CXX_TEST_PATH}/InfovisBoostArrayCxxTests ${TName})
    ENDFOREACH(test)

  ENDIF(VTK_USE_BOOST)
ENDIF(VTK_USE_N_WAY_ARRAYS)

