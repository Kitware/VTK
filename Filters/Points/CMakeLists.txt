set(Module_SRCS
  vtkEllipsoidalGaussianKernel.cxx
  vtkGaussianKernel.cxx
  vtkInterpolationKernel.cxx
  vtkPointInterpolator.cxx
  vtkLinearKernel.cxx
  vtkSPHKernel.cxx
  vtkShepardKernel.cxx
  vtkVoronoiKernel.cxx
  )

set_source_files_properties(
  vtkInterpolationKernel
  ABSTRACT
  )

vtk_module_library(vtkFiltersPoints ${Module_SRCS})
