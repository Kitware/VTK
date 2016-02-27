set(Module_SRCS
  vtkArrayListTemplate.txx
  vtkEllipsoidalGaussianKernel.cxx
  vtkGaussianKernel.cxx
  vtkInterpolationKernel.cxx
  vtkPointInterpolator.cxx
  vtkLinearKernel.cxx
  vtkSPHKernel.cxx
  vtkShepardKernel.cxx
  vtkVoronoiKernel.cxx
  )

set(${vtk-module}_HDRS
  vtkArrayListTemplate.h
  )

set_source_files_properties(
  vtkInterpolationKernel
  ABSTRACT
  )

set_source_files_properties(
  vtkArrayListTemplate.txx
  WRAP_EXCLUDE
)
vtk_module_library(vtkFiltersPoints ${Module_SRCS})
