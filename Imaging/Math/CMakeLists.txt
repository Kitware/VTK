set(Module_SRCS
  vtkImageDivergence.cxx
  vtkImageDotProduct.cxx
  vtkImageLogarithmicScale.cxx
  vtkImageLogic.cxx
  vtkImageMagnitude.cxx
  vtkImageMaskBits.cxx
  vtkImageMathematics.cxx
  vtkImageWeightedSum.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
