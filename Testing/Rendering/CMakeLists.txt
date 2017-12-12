set(module_SRCS
  vtkMultiBaselineRegressionTest.h
  vtkRegressionTestImage.h
  vtkTesting.cxx
  vtkTestingInteractor.cxx
  vtkTestingObjectFactory.cxx
)

vtk_module_library(vtkTestingRendering ${module_SRCS})
