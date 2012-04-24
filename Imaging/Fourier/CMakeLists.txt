set(Module_SRCS
  vtkImageButterworthHighPass.cxx
  vtkImageButterworthLowPass.cxx
  vtkImageFFT.cxx
  vtkImageFourierCenter.cxx
  vtkImageFourierFilter.cxx
  vtkImageIdealHighPass.cxx
  vtkImageIdealLowPass.cxx
  vtkImageRFFT.cxx
  )

set_source_files_properties(
  vtkImageFourierFilter
  ABSTRACT
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
