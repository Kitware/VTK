set(Module_SRCS
  vtkEllipsoidalGaussianKernel.cxx
  vtkGaussianKernel.cxx
  vtkGeneralizedKernel.cxx
  vtkInterpolationKernel.cxx
  vtkLinearKernel.cxx
  vtkPointInterpolator.cxx
  vtkPointInterpolator2D.cxx
  vtkProbabilisticVoronoiKernel.cxx
  vtkSPHKernel.cxx
  vtkShepardKernel.cxx
  vtkVoronoiKernel.cxx
  )

set_source_files_properties(
  vtkGeneralizedKernel.cxx
  vtkInterpolationKernel
  ABSTRACT
  )

vtk_module_library(vtkFiltersPoints ${Module_SRCS})
