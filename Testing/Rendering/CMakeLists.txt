set(classes
  vtkTesting
  vtkTestingInteractor
  vtkTestingObjectFactory)

set(headers
  vtkMultiBaselineRegressionTest.h
  vtkRegressionTestImage.h)

vtk_module_add_module(VTK::TestingRendering
  CLASSES ${classes}
  HEADERS ${headers})
