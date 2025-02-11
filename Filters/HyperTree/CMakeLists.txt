set(classes
  vtkHyperTreeGridAxisClip
  vtkHyperTreeGridAxisCut
  vtkHyperTreeGridAxisReflection
  vtkHyperTreeGridCellCenters
  vtkHyperTreeGridCellSizeStrategy
  vtkHyperTreeGridContour
  vtkHyperTreeGridDepthLimiter
  vtkHyperTreeGridEvaluateCoarse
  vtkHyperTreeGridExtractGhostCells
  vtkHyperTreeGridFeatureEdges
  vtkHyperTreeGridGenerateFields
  vtkHyperTreeGridGeometry
  vtkHyperTreeGridGradient
  vtkHyperTreeGridPlaneCutter
  vtkHyperTreeGridRemoveGhostCells
  vtkHyperTreeGridThreshold
  vtkHyperTreeGridToDualGrid
  vtkHyperTreeGridToUnstructuredGrid
  vtkHyperTreeGridValidCellStrategy
  vtkHyperTreeGridVisibleLeavesSize
  vtkImageDataToHyperTreeGrid
)

set(headers
  vtkHyperTreeGridGenerateFieldStrategy.h
)

set(private_classes
  vtkHyperTreeGridGeometryImpl
  vtkHyperTreeGridGeometry1DImpl
  vtkHyperTreeGridGeometry2DImpl
  vtkHyperTreeGridGeometry3DImpl
  vtkHyperTreeGridGeometrySmallDimensionsImpl
)

vtk_module_add_module(VTK::FiltersHyperTree
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_CLASSES ${private_classes})
vtk_add_test_mangling(VTK::FiltersHyperTree)
