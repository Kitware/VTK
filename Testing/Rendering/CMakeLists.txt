set(module_SRCS
  vtkTesting.cxx
  vtkTestingInteractor.cxx
  vtkTestingObjectFactory.cxx
  )

vtk_module_library(vtkTestingRendering ${module_SRCS})
