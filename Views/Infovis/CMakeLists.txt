SET(Module_SRCS
  vtkApplyColors.cxx
  vtkApplyIcons.cxx
  vtkGraphItem.cxx
  vtkGraphLayoutView.cxx
  vtkHierarchicalGraphPipeline.cxx
  vtkHierarchicalGraphView.cxx
  vtkIcicleView.cxx
  vtkInteractorStyleAreaSelectHover.cxx
  vtkInteractorStyleTreeMapHover.cxx
  vtkParallelCoordinatesHistogramRepresentation.cxx
  vtkParallelCoordinatesRepresentation.cxx
  vtkParallelCoordinatesView.cxx
  vtkRenderedGraphRepresentation.cxx
  vtkRenderedHierarchyRepresentation.cxx
  vtkRenderedRepresentation.cxx
  vtkRenderedSurfaceRepresentation.cxx
  vtkRenderedTreeAreaRepresentation.cxx
  vtkRenderView.cxx
  vtkTreeAreaView.cxx
  vtkTreeHeatmapItem.cxx
  vtkTreeMapView.cxx
  vtkTreeRingView.cxx
  vtkViewUpdater.cxx
  )

vtk_module_library(vtkViewsInfovis ${Module_SRCS})
