set(classes
  vtkTesting
  vtkTestingInteractor
  vtkTestingObjectFactory)

set(headers
  vtkMultiBaselineRegressionTest.h)

set(nowrap_headers
  vtkRegressionTestImage.h)

vtk_module_add_module(VTK::TestingRendering
  CLASSES ${classes}
  HEADERS ${headers}
  NOWRAP_HEADERS ${nowrap_headers})
