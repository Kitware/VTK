set(Module_SRCS
  vtkStreamTracer.cxx
  vtkModifiedBSPTree.cxx
  vtkCellLocatorInterpolatedVelocityField.cxx
  )

vtk_module_library(vtkFiltersTracers ${Module_SRCS})
