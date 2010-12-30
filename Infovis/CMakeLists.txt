# Note: If you are looking at this file in response to a warning
# message about an outdated Boost version, please search for the
# string BOOST_OLD_VERSION.

SET(KIT Infovis)
SET(UKIT INFOVIS)
IF(VTK_USE_RENDERING)
  SET(KIT_TCL_LIBS vtkWidgetsTCL)
  SET(KIT_PYTHON_LIBS vtkWidgetsPythonD)
  SET(KIT_JAVA_LIBS vtkWidgetsJava)
  SET(KIT_INTERFACE_LIBRARIES vtkWidgets)
  SET(KIT_LIBS ${VTK_LIBXML2_LIBRARIES} vtkalglib)
ELSE(VTK_USE_RENDERING)
  SET(KIT_TCL_LIBS vtkGraphicsTCL)
  SET(KIT_PYTHON_LIBS vtkGraphicsPythonD)
  SET(KIT_JAVA_LIBS vtkGraphicsJava)
  SET(KIT_INTERFACE_LIBRARIES vtkGraphics)
  SET(KIT_LIBS ${VTK_LIBXML2_LIBRARIES} vtkalglib vtkIO vtkImaging)
ENDIF(VTK_USE_RENDERING)

SET( Kit_SRCS
vtkAddMembershipArray.cxx
vtkAppendPoints.cxx
vtkApplyColors.cxx
vtkApplyIcons.cxx
vtkArcParallelEdgeStrategy.cxx
vtkAreaLayout.cxx
vtkAreaLayoutStrategy.cxx
vtkAssignCoordinates.cxx
vtkAssignCoordinatesLayoutStrategy.cxx
vtkBivariateLinearTableThreshold.cxx
vtkBivariateStatisticsAlgorithm.cxx
vtkBoxLayoutStrategy.cxx
vtkChacoGraphReader.cxx
vtkCircularLayoutStrategy.cxx
vtkClustering2DLayoutStrategy.cxx
vtkCollapseGraph.cxx
vtkCollapseVerticesByArray.cxx
vtkCommunity2DLayoutStrategy.cxx
vtkConeLayoutStrategy.cxx
vtkConstrained2DLayoutStrategy.cxx
vtkContingencyStatistics.cxx
vtkCorrelativeStatistics.cxx
vtkCosmicTreeLayoutStrategy.cxx
vtkDataObjectToTable.cxx
vtkDelimitedTextReader.cxx
vtkDescriptiveStatistics.cxx
vtkDIMACSGraphReader.cxx
vtkDIMACSGraphWriter.cxx
vtkEdgeCenters.cxx
vtkEdgeLayout.cxx
vtkEdgeLayoutStrategy.cxx
vtkExpandSelectedGraph.cxx
vtkExtractSelectedGraph.cxx
vtkFast2DLayoutStrategy.cxx
vtkFixedWidthTextReader.cxx
vtkForceDirectedLayoutStrategy.cxx
vtkGenerateIndexArray.cxx
vtkGeoEdgeStrategy.cxx
vtkGeoMath.cxx
vtkGraphHierarchicalBundle.cxx
vtkGraphHierarchicalBundleEdges.cxx
vtkGraphLayout.cxx
vtkGraphLayoutStrategy.cxx
vtkGroupLeafVertices.cxx
vtkISIReader.cxx
vtkKMeansDistanceFunctor.cxx
vtkKMeansDistanceFunctorCalculator.cxx
vtkKMeansStatistics.cxx
vtkMergeColumns.cxx
vtkMergeGraphs.cxx
vtkMergeTables.cxx
vtkMultiCorrelativeStatistics.cxx
vtkMutableGraphHelper.cxx
vtkNetworkHierarchy.cxx
vtkOrderStatistics.cxx
vtkPassArrays.cxx
vtkPassThrough.cxx
vtkPassThroughEdgeStrategy.cxx
vtkPassThroughLayoutStrategy.cxx
vtkPerturbCoincidentVertices.cxx
vtkPCAStatistics.cxx
vtkPruneTreeFilter.cxx
vtkRISReader.cxx
vtkRandomGraphSource.cxx
vtkRandomLayoutStrategy.cxx
vtkRemoveIsolatedVertices.cxx
vtkRemoveHiddenData.cxx
vtkSCurveSpline.cxx
vtkSimple2DLayoutStrategy.cxx
vtkSimple3DCirclesStrategy.cxx
vtkSliceAndDiceLayoutStrategy.cxx
vtkSplineGraphEdges.cxx
vtkSplitColumnComponents.cxx
vtkSquarifyLayoutStrategy.cxx
vtkSpanTreeLayoutStrategy.cxx
vtkStackedTreeLayoutStrategy.cxx
vtkStatisticsAlgorithm.cxx
vtkStrahlerMetric.cxx
vtkStreamGraph.cxx
vtkStreamingStatistics.cxx
vtkStringToCategory.cxx
vtkStringToNumeric.cxx
vtkTableToGraph.cxx
vtkTableToTreeFilter.cxx
vtkThresholdGraph.cxx
vtkThresholdTable.cxx
vtkTransferAttributes.cxx
vtkTreeFieldAggregator.cxx
vtkTreeLayoutStrategy.cxx
vtkTreeOrbitLayoutStrategy.cxx
vtkTreeLevelsFilter.cxx
vtkTreeMapLayout.cxx
vtkTreeMapLayoutStrategy.cxx
vtkTreeMapToPolyData.cxx
vtkTreeRingToPolyData.cxx
vtkTulipReader.cxx
vtkUnivariateStatisticsAlgorithm.cxx
vtkAttributeClustering2DLayoutStrategy.cxx
vtkVertexDegree.cxx
vtkXGMLReader.cxx
vtkXMLTreeReader.cxx
)

# Add these classes only if using Boost

# About the BOOST_OLD_VERSION warning:
#
# The Kruskal minimum spanning tree algorithm does not compile
# correctly with Boost versions earlier than 1.36 on 64-bit platforms.
# If you're using one of those old versions you have two options:
#
#
# 1.  Download a new version of Boost from http://www.boost.org and
#     install it wherever you like.  Make sure to change the CMAKE
#     variable Boost_INCLUDE_DIR to point to the new version.
#
# 2.  Continue with your existing version of Boost.  We will still
#     compile whatever Boost filters we can.
#
# Upgrading to a recent Boost version is the preferred solution.  A
# future version of ParaView will probably require Boost 1.36 or later.


IF(VTK_USE_BOOST)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  IF( "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}" LESS 1.36)
    IF( HAVE_CMAKE_SIZEOF_VOID_P AND CMAKE_SIZEOF_VOID_P EQUAL 8 )
      SET( BOOST_OLD_VERSION TRUE )
      MESSAGE( "Your system is using an outdated version of Boost ("
               "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}"
               ", found in ${Boost_INCLUDE_DIR}).  Some of the graph "
               "processing filters require version 1.36.0 or later to "
               "build on 64-bit platforms.  \n\nThe system will still compile "
               "but those filters will be disabled.  Please see "
               "ParaView3/VTK/Infovis/CMakeLists.txt "
               "for more information." )
    ENDIF( HAVE_CMAKE_SIZEOF_VOID_P AND CMAKE_SIZEOF_VOID_P EQUAL 8 )
  ENDIF( "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}" LESS 1.36 )

  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkBoostBetweennessClustering.cxx
    vtkBoostBiconnectedComponents.cxx
    vtkBoostBrandesCentrality.cxx
    vtkBoostBreadthFirstSearch.cxx
    vtkBoostBreadthFirstSearchTree.cxx
    vtkBoostConnectedComponents.cxx
    vtkBoostExtractLargestComponent.cxx
    vtkBoostPrimMinimumSpanningTree.cxx
    vtkBoostSplitTableField.cxx
  )

  IF( NOT BOOST_OLD_VERSION )
    SET( Kit_SRCS
        ${Kit_SRCS}
        vtkBoostKruskalMinimumSpanningTree.cxx
       )
  ENDIF( NOT BOOST_OLD_VERSION )

ENDIF(VTK_USE_BOOST)

IF(VTK_USE_N_WAY_ARRAYS)

  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkAdjacencyMatrixToEdgeTable.cxx
    vtkArrayNorm.cxx
    vtkArrayToTable.cxx
    vtkComputeHistogram2DOutliers.cxx
    vtkDiagonalMatrixSource.cxx
    vtkDotProductSimilarity.cxx
    vtkExtractArray.cxx
    vtkExtractHistogram2D.cxx
    vtkMatricizeArray.cxx
    vtkNormalizeMatrixVectors.cxx
    vtkPairwiseExtractHistogram2D.cxx
    vtkSparseArrayToTable.cxx
    vtkTableToArray.cxx
    vtkTableToSparseArray.cxx
    vtkTransposeMatrix.cxx
    )

  IF(VTK_USE_BOOST)

    SET(Kit_SRCS
      ${Kit_SRCS}
      vtkBoostLogWeighting.cxx
      vtkBoostRandomSparseArraySource.cxx
    )

  ENDIF(VTK_USE_BOOST)
ENDIF(VTK_USE_N_WAY_ARRAYS)

# Add these classes only if PARALLEL is turned ON
IF(VTK_USE_PARALLEL)
  SET(Kit_SRCS ${Kit_SRCS}
    vtkPBivariateLinearTableThreshold.cxx
    vtkPCorrelativeStatistics.cxx
    vtkPContingencyStatistics.cxx
    vtkPDescriptiveStatistics.cxx
    vtkPKMeansStatistics.cxx
    vtkPMultiCorrelativeStatistics.cxx
    vtkPOrderStatistics.cxx
    vtkPPCAStatistics.cxx
    )

    IF(VTK_USE_N_WAY_ARRAYS)
      SET(Kit_SRCS ${Kit_SRCS}
        vtkPComputeHistogram2DOutliers.cxx
        vtkPExtractHistogram2D.cxx
        vtkPPairwiseExtractHistogram2D.cxx
        )
    ENDIF(VTK_USE_N_WAY_ARRAYS)

  SET(KIT_INTERFACE_LIBRARIES ${KIT_INTERFACE_LIBRARIES} vtkParallel)
ENDIF(VTK_USE_PARALLEL)

# Add these classes only if vtkTypeUInt64Array exists
IF(VTK_HAS_UINT64_ARRAY)
  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkSQLDatabaseGraphSource.cxx
    vtkSQLDatabaseTableSource.cxx
    vtkSQLGraphReader.cxx    # depends on vtkRowQueryToTable, which depends on uint64 array
    vtkStringToTimePoint.cxx
    vtkTimePointToString.cxx
  )
ENDIF(VTK_HAS_UINT64_ARRAY)

SET_SOURCE_FILES_PROPERTIES(
vtkAreaLayoutStrategy.cxx
vtkBivariateStatisticsAlgorithm.cxx
vtkEdgeLayoutStrategy.cxx
vtkGraphLayoutStrategy.cxx
vtkStatisticsAlgorithm.cxx
vtkTreeMapLayoutStrategy.cxx
vtkUnivariateStatisticsAlgorithm.cxx
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
