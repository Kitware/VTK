set(module_SRCS
  vtkRegressionTestImage.h
  vtkTesting.cxx
  vtkTestingInteractor.cxx
  vtkTestingObjectFactory.cxx
  )

set_source_files_properties(vtkRegressionTestImage WRAP_EXCLUDE)

vtk_module_library(vtkTestingRendering ${module_SRCS})
