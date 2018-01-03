set(classes
  vtkHyperTreeGridAxisClip
  vtkHyperTreeGridAxisCut
  vtkHyperTreeGridAxisReflection
  vtkHyperTreeGridCellCenters
  vtkHyperTreeGridContour
  vtkHyperTreeGridDepthLimiter
  vtkHyperTreeGridEvaluateCoarse
  vtkHyperTreeGridGeometry
  vtkHyperTreeGridPlaneCutter
  vtkHyperTreeGridThreshold
  vtkHyperTreeGridToUnstructuredGrid)

vtk_module_add_module(VTK::FiltersHyperTree
  CLASSES ${classes})
