SET(KIT Infovis)
SET(UKIT INFOVIS)
SET(KIT_TCL_LIBS vtkWidgetsTCL)
SET(KIT_PYTHON_LIBS vtkWidgetsPythonD)
SET(KIT_JAVA_LIBS vtkWidgetsJava)
SET(KIT_LIBS vtkWidgets vtklibxml2)

SET( Kit_SRCS
vtkBoxLayoutStrategy.cxx
vtkCircularLayoutStrategy.cxx
vtkDataObjectToTable.cxx
vtkDelimitedTextReader.cxx
vtkExtractSelectedGraph.cxx
vtkFast2DLayoutStrategy.cxx
vtkFixedWidthTextReader.cxx
vtkForceDirectedLayoutStrategy.cxx
vtkGraphHierarchicalBundle.cxx
vtkGraphLayout.cxx
vtkGraphLayoutStrategy.cxx
vtkGraphLayoutViewer.cxx
vtkGraphToPolyData.cxx
vtkGroupLeafVertices.cxx
vtkInteractorStyleTreeMapHover.cxx
vtkLabeledTreeMapDataMapper.cxx
vtkMergeColumns.cxx
vtkMergeTables.cxx
vtkPruneTreeFilter.cxx
vtkRandomGraphSource.cxx
vtkRandomLayoutStrategy.cxx
vtkSimple2DLayoutStrategy.cxx
vtkSliceAndDiceLayoutStrategy.cxx
vtkSquarifyLayoutStrategy.cxx
vtkStringToNumeric.cxx
vtkTableToGraphFilter.cxx
vtkTableToTreeFilter.cxx
vtkThresholdTable.cxx
vtkTreeFieldAggregator.cxx
vtkTreeLayoutStrategy.cxx
vtkTreeLevelsFilter.cxx
vtkTreeMapLayout.cxx
vtkTreeMapLayoutStrategy.cxx
vtkTreeMapToPolyData.cxx
vtkTreeMapViewer.cxx
vtkVertexDegree.cxx
vtkXMLTreeReader.cxx
)

# Add the option to use Boost
OPTION(VTK_USE_BOOST "Use Boost libraries for graph algorithms - www.boost.org." OFF)

# Look for Boost only if using Boost
IF(VTK_USE_BOOST)
  FIND_PACKAGE(Boost REQUIRED)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
ENDIF(VTK_USE_BOOST)

# Add these classes only if using Boost
IF(VTK_USE_BOOST)
  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkBoostBiconnectedComponents.cxx
    vtkBoostBrandesCentrality.cxx
    vtkBoostBreadthFirstSearch.cxx
    vtkBoostBreadthFirstSearchTree.cxx
    vtkBoostConnectedComponents.cxx
  )
ENDIF(VTK_USE_BOOST)

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
