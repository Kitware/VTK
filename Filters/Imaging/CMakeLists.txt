set(Module_SRCS
  vtkPairwiseExtractHistogram2D.cxx
  vtkExtractHistogram2D.cxx
  vtkComputeHistogram2DOutliers.cxx
  )

vtk_module_library(vtkFiltersImaging ${Module_SRCS})
