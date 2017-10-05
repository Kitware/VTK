set(module_SRCS
  vtkMultiBaselineRegressionTest.h
  vtkRegressionTestImage.h
  vtkTesting.cxx
  vtkTestingInteractor.cxx
  vtkTestingObjectFactory.cxx
)

set_source_files_properties(
  vtkMultiBaselineRegressionTest.h
  vtkRegressionTestImage
  PROPERTIES
    WRAP_EXCLUDE TRUE
)

vtk_module_library(vtkTestingRendering ${module_SRCS})
