set(Module_SRCS
  vtkImageHSIToRGB.cxx
  vtkImageHSVToRGB.cxx
  vtkImageLuminance.cxx
  vtkImageMapToRGBA.cxx
  vtkImageMapToWindowLevelColors.cxx
  vtkImageQuantizeRGBToIndex.cxx
  vtkImageRGBToHSI.cxx
  vtkImageRGBToHSV.cxx
  vtkImageRGBToYIQ.cxx
  vtkImageYIQToRGB.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
