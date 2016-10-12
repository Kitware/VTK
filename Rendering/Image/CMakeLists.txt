set(Module_SRCS
  vtkDepthImageToPointCloud.cxx
  vtkImageResliceMapper.cxx
  vtkImageSliceCollection.cxx
  vtkImageStack.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
