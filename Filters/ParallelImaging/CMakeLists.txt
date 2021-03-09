set(classes
  vtkExtractPiece
  vtkMemoryLimitImageDataStreamer
  vtkPComputeHistogram2DOutliers
  vtkPExtractHistogram2D
  vtkPPairwiseExtractHistogram2D
  vtkTransmitImageDataPiece)

vtk_module_add_module(VTK::FiltersParallelImaging
  CLASSES ${classes})
