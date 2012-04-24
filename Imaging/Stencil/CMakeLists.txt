set(Module_SRCS
  vtkImageStencil.cxx
  vtkImageStencilToImage.cxx
  vtkImageToImageStencil.cxx
  vtkImplicitFunctionToImageStencil.cxx
  vtkLassoStencilSource.cxx
  vtkPolyDataToImageStencil.cxx
  vtkROIStencilSource.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
