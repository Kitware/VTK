set(classes
  vtkHyperTreeGridAxisClip
  vtkHyperTreeGridAxisCut
  vtkHyperTreeGridAxisReflection
  vtkHyperTreeGridCellCenters
  vtkHyperTreeGridContour
  vtkHyperTreeGridDepthLimiter
  vtkHyperTreeGridEvaluateCoarse
  vtkHyperTreeGridEvaluateCoarse
  vtkHyperTreeGridGeometry
  vtkImageDataToHyperTreeGrid
  vtkHyperTreeGridPlaneCutter
  vtkHyperTreeGridThreshold
  vtkHyperTreeGridToDualGrid
  vtkHyperTreeGridToUnstructuredGrid
)

vtk_module_add_module(VTK::FiltersHyperTree
  CLASSES ${classes})
