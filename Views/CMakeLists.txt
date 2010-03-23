SET(KIT Views)
SET(UKIT VIEWS)
SET(KIT_TCL_LIBS vtkInfovisTCL)
SET(KIT_PYTHON_LIBS vtkInfovisPythonD)
SET(KIT_JAVA_LIBS vtkInfovisJava)
SET(KIT_INTERFACE_LIBRARIES vtkInfovis)

SET( Kit_SRCS
vtkConvertSelectionDomain.cxx
vtkDataRepresentation.cxx
vtkEmptyRepresentation.cxx
vtkGraphLayoutView.cxx
vtkHierarchicalGraphPipeline.cxx
vtkHierarchicalGraphView.cxx
vtkIcicleView.cxx
vtkInteractorStyleAreaSelectHover.cxx
vtkInteractorStyleTreeMapHover.cxx
vtkRenderedSurfaceRepresentation.cxx
vtkRenderedGraphRepresentation.cxx
vtkRenderedRepresentation.cxx
vtkRenderedTreeAreaRepresentation.cxx
vtkRenderedHierarchyRepresentation.cxx
vtkRenderView.cxx
vtkTreeAreaView.cxx
vtkTreeMapView.cxx
vtkTreeRingView.cxx
vtkView.cxx
vtkViewUpdater.cxx
)

IF(VTK_USE_N_WAY_ARRAYS)
  SET(Kit_SRCS
    ${Kit_SRCS}    
    vtkParallelCoordinatesHistogramRepresentation.cxx
    vtkParallelCoordinatesRepresentation.cxx
    vtkParallelCoordinatesView.cxx
    )
ENDIF(VTK_USE_N_WAY_ARRAYS)

IF(VTK_USE_BOOST)
  SET(Kit_SRCS
    ${Kit_SRCS}
    vtkPipelineGraphSource.cxx
    )
ENDIF(VTK_USE_BOOST)

#SET_SOURCE_FILES_PROPERTIES(
#vtkView.cxx
#ABSTRACT
#)

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
