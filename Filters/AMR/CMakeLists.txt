set(Module_SRCS
  vtkAMRCutPlane.cxx
  vtkAMRGaussianPulseSource.cxx
  vtkAMRResampleFilter.cxx
  vtkAMRSliceFilter.cxx
  vtkAMRToMultiBlockFilter.cxx
  vtkImageToAMR.cxx
  vtkParallelAMRUtilities.cxx
  )

vtk_module_library(vtkFiltersAMR ${Module_SRCS})
