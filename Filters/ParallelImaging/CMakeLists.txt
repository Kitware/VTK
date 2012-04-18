set(Module_SRCS
  vtkPComputeHistogram2DOutliers.cxx
  vtkPExtractHistogram2D.cxx
  vtkPPairwiseExtractHistogram2D.cxx
  vtkExtractPiece.cxx
  vtkMemoryLimitImageDataStreamer.cxx
  vtkTransmitImageDataPiece.cxx
  )

vtk_module_library(vtkFiltersParallelImaging ${Module_SRCS})
