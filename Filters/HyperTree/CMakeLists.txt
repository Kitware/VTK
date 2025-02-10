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

set(private_classes
  vtkHyperTreeGridGenerateFieldCellSize
  vtkHyperTreeGridGenerateFieldValidCell
  vtkHyperTreeGridGeometryImpl
  vtkHyperTreeGridGeometry1DImpl
  vtkHyperTreeGridGeometry2DImpl
  vtkHyperTreeGridGeometry3DImpl
  vtkHyperTreeGridGeometrySmallDimensionsImpl
)

set(private_headers
  vtkHyperTreeGridGenerateField.h
)

vtk_module_add_module(VTK::FiltersHyperTree
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers})
vtk_add_test_mangling(VTK::FiltersHyperTree)
