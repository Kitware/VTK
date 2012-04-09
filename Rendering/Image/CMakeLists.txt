set(Module_SRCS
  vtkImageResliceMapper.cxx
  vtkImageSliceCollection.cxx
  vtkImageStack.cxx
  vtkImageViewer2.cxx
  vtkImageViewer.cxx
  vtkResliceImageViewer.cxx
  vtkResliceImageViewerMeasurements.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
