set(classes
  vtkComputeHistogram2DOutliers
  vtkExtractHistogram2D
  vtkPairwiseExtractHistogram2D)

vtk_module_add_module(VTK::FiltersImaging
  CLASSES ${classes})
