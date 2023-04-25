set(classes
  vtkHyperTreeGridAxisClip
  vtkHyperTreeGridAxisCut
  vtkHyperTreeGridAxisReflection
  vtkHyperTreeGridCellCenters
  vtkHyperTreeGridContour
  vtkHyperTreeGridDepthLimiter
  vtkHyperTreeGridEvaluateCoarse
  vtkHyperTreeGridGeometry
  vtkHyperTreeGridGeometryInternal
  vtkHyperTreeGridGeometryInternal1D
  vtkHyperTreeGridGeometryInternal2D
  vtkHyperTreeGridGeometryInternal3D
  vtkHyperTreeGridGradient
  vtkHyperTreeGridPlaneCutter
  vtkHyperTreeGridThreshold
  vtkHyperTreeGridToDualGrid
  vtkHyperTreeGridToUnstructuredGrid
  vtkImageDataToHyperTreeGrid
)

vtk_module_add_module(VTK::FiltersHyperTree
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersHyperTree)
