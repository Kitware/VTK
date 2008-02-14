# if we have rendering add the following tests
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  SET(KIT Infovis)
  # add tests that do not require data
  SET(MyTests  
    TestAssignCoordinates
    TestDataObjectToTable
    TestExtractSelectedGraph
    TestGraph
    TestGraphAlgorithms   
    TestGraphHierarchicalBundle
    TestGraphLayoutStrategy
    TestGroupLeafVertices
    TestInteractorStyleTreeMapHover
    TestKdTreeBoxSelection
    TestPassThrough
    TestPruneTreeFilter
    TestRandomGraphSource
    TestStringToCategory
    TestTable
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
      TestISIReader
      TestFixedWidthTextReader
      TestRISReader
      TestStringToNumeric
      TestTableToGraph
      TestTreeMapViewer
      TestTulipReader
      )
  ENDIF (VTK_DATA_ROOT)
  IF (VTK_USE_BOOST)
    # add boost tests
    SET(MyTests ${MyTests}
      TestBoostAdapter
      TestBoostAlgorithms
      )

    IF(VTK_DATA_ROOT)
      SET(MyTests ${MyTests}
        TestBoostSplitTableField
        )
    ENDIF(VTK_DATA_ROOT)
  ENDIF (VTK_USE_BOOST)
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
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkInfovis vtkRendering vtkIO vtkCommon)
  SET (TestsToRun ${Tests})
  REMOVE (TestsToRun ${KIT}CxxTests.cxx)

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
