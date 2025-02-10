set(classes
  vtkHyperTreeGridAxisClip
  vtkHyperTreeGridAxisCut
  vtkHyperTreeGridAxisReflection
  vtkHyperTreeGridCellCenters
  vtkHyperTreeGridContour
  vtkHyperTreeGridDepthLimiter
  vtkHyperTreeGridEvaluateCoarse
  vtkHyperTreeGridExtractGhostCells
  vtkHyperTreeGridFeatureEdges
  vtkHyperTreeGridGenerateFields
  vtkHyperTreeGridGenerateFieldCellSize
  vtkHyperTreeGridGenerateFieldValidCell
  vtkHyperTreeGridGeometry
  vtkHyperTreeGridGradient
  vtkHyperTreeGridPlaneCutter
  vtkHyperTreeGridRemoveGhostCells
  vtkHyperTreeGridThreshold
  vtkHyperTreeGridToDualGrid
  vtkHyperTreeGridToUnstructuredGrid
  vtkHyperTreeGridVisibleLeavesSize
  vtkImageDataToHyperTreeGrid
)

set(headers
  vtkHyperTreeGridGenerateField.h
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
