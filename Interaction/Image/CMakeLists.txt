set(Module_SRCS
  vtkImageViewer2.cxx
  vtkImageViewer.cxx
  vtkResliceImageViewer.cxx
  vtkResliceImageViewerMeasurements.cxx
  )

vtk_module_library(${vtk-module} ${Module_SRCS})
