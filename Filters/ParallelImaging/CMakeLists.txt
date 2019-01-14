set(classes
  vtkPComputeHistogram2DOutliers
  vtkPExtractHistogram2D
  vtkPPairwiseExtractHistogram2D
  vtkExtractPiece
  vtkMemoryLimitImageDataStreamer
  vtkTransmitImageDataPiece)

vtk_module_add_module(VTK::FiltersParallelImaging
  CLASSES ${classes})
