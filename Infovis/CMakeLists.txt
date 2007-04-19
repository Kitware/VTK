SET(KIT Infovis)
SET(UKIT INFOVIS)
SET(KIT_TCL_LIBS vtkWidgetsTCL)
SET(KIT_PYTHON_LIBS vtkWidgetsPythonD)
SET(KIT_JAVA_LIBS vtkWidgetsJava)
SET(KIT_LIBS vtkWidgets)

SET( Kit_SRCS
vtkBoxLayoutStrategy.cxx
vtkDelimitedTextReader.cxx
vtkExtractSelectedGraph.cxx
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
vtkPruneTreeFilter.cxx
vtkRandomGraphSource.cxx
vtkRandomLayoutStrategy.cxx
vtkSimple2DLayoutStrategy.cxx
vtkSliceAndDiceLayoutStrategy.cxx
vtkSquarifyLayoutStrategy.cxx
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
)

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
