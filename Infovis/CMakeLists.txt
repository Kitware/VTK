SET(KIT Infovis)
SET(UKIT INFOVIS)
SET(KIT_TCL_LIBS vtkWidgetsTCL)
SET(KIT_PYTHON_LIBS vtkWidgetsPythonD)
SET(KIT_JAVA_LIBS vtkWidgetsJava)
SET(KIT_LIBS vtkWidgets ${VTK_LIBXML2_LIBRARIES})

SET( Kit_SRCS
vtkArrayMap.cxx
vtkAssignCoordinates.cxx
vtkBoxLayoutStrategy.cxx
vtkChacoGraphReader.cxx
vtkCircularLayoutStrategy.cxx
vtkClustering2DLayoutStrategy.cxx
vtkCommunity2DLayoutStrategy.cxx
vtkConstrained2DLayoutStrategy.cxx
vtkDataObjectToTable.cxx
vtkDelimitedTextReader.cxx
vtkEdgeCenters.cxx
vtkExtractSelectedGraph.cxx
vtkFast2DLayoutStrategy.cxx
vtkFixedWidthTextReader.cxx
vtkForceDirectedLayoutStrategy.cxx
vtkGraphHierarchicalBundle.cxx
vtkGraphLayout.cxx
vtkGraphLayoutStrategy.cxx
vtkGraphMapper.cxx
vtkGraphToPolyData.cxx
vtkGroupLeafVertices.cxx
vtkISIReader.cxx
vtkInteractorStyleTreeMapHover.cxx
vtkLabeledTreeMapDataMapper.cxx
vtkMergeColumns.cxx
vtkMergeTables.cxx
vtkPassThrough.cxx
vtkPassThroughLayoutStrategy.cxx
vtkPruneTreeFilter.cxx
vtkRISReader.cxx
vtkRandomGraphSource.cxx
vtkRandomLayoutStrategy.cxx
vtkSimple2DLayoutStrategy.cxx
vtkSliceAndDiceLayoutStrategy.cxx
vtkSquarifyLayoutStrategy.cxx
vtkStringToCategory.cxx
vtkStringToNumeric.cxx
vtkTableToGraph.cxx
vtkTableToTreeFilter.cxx
vtkThresholdTable.cxx
vtkTimePointUtility.cxx
vtkTreeFieldAggregator.cxx
vtkTreeLayoutStrategy.cxx
vtkTreeOrbitLayoutStrategy.cxx
vtkTreeLevelsFilter.cxx
vtkTreeMapLayout.cxx
vtkTreeMapLayoutStrategy.cxx
vtkTreeMapToPolyData.cxx
vtkTreeMapViewer.cxx
vtkTulipReader.cxx
vtkVertexDegree.cxx
vtkViewTheme.cxx
vtkXMLTreeReader.cxx
)

# Add these classes only if using Boost
IF(VTK_USE_BOOST)
  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkBoostBiconnectedComponents.cxx
    vtkBoostBrandesCentrality.cxx
    vtkBoostBreadthFirstSearch.cxx
    vtkBoostBreadthFirstSearchTree.cxx
    vtkBoostConnectedComponents.cxx
    vtkBoostSplitTableField.cxx
  )
ENDIF(VTK_USE_BOOST)

# Add these classes only if vtkTypeUInt64Array exists
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
  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkSQLGraphReader.cxx    # depends on vtkRowQueryToTable, which depends on uint64 array
    vtkStringToTimePoint.cxx
    vtkTimePointToString.cxx
  )
ENDIF(VTK_HAS_UINT64_ARRAY)

SET_SOURCE_FILES_PROPERTIES(
vtkGraphLayoutStrategy.cxx
vtkTreeMapLayoutStrategy.cxx
ABSTRACT
)

SET(Kit_EXTRA_SRCS)
SET(Kit_EXTRA_CMDS)
SET(Kit_TCL_EXTRA_SRCS)
SET(Kit_PYTHON_EXTRA_SRCS)
SET(Kit_JAVA_EXTRA_SRCS)
SET(KIT_TCL_DEPS)
SET(KIT_PYTHON_DEPS)
SET(KIT_JAVA_DEPS)

#-----------------------------------------------------------------------------
# Include CMake code common to all kits.
INCLUDE(${VTK_CMAKE_DIR}/KitCommonBlock.cmake)
#-----------------------------------------------------------------------------
