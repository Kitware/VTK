set(Module_SRCS
  vtkImageFlip.cxx
  vtkImageFFT.cxx
  vtkImageFourierCenter.cxx
  vtkImageFourierFilter.cxx
  vtkImageRFFT.cxx
  )

set_source_files_properties(
  vtkImageFourierFilter
  ABSTRACT
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
