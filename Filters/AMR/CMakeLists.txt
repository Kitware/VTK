set(Module_SRCS
  vtkAMRCutPlane.cxx
  vtkAMRGaussianPulseSource.cxx
  vtkAMRResampleFilter.cxx
  vtkAMRSliceFilter.cxx
  vtkAMRToMultiBlockFilter.cxx
  vtkAMRUtilities.cxx
  vtkImageToAMR.cxx
  )

set_source_files_properties(
  vtkAMRBaseReader
  vtkAMRBaseParticlesReader
  ABSTRACT
  )

set_source_files_properties(
  vtkAMRUtilities
  WRAP_EXCLUDE
  )

vtk_module_library(vtkFiltersAMR ${Module_SRCS})
