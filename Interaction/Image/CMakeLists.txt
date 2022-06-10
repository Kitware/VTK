set(classes
  vtkImageViewer
  vtkImageViewer2
  vtkResliceImageViewer
  vtkResliceImageViewerMeasurements
  )

vtk_module_add_module(VTK::InteractionImage
  CLASSES ${classes})
vtk_add_test_mangling(VTK::InteractionImage)
