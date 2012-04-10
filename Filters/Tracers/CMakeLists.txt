set(Module_SRCS
  vtkAbstractInterpolatedVelocityField.cxx
  vtkDashedStreamLine.cxx
  vtkInterpolatedVelocityField.cxx
  vtkStreamer.cxx
  vtkStreamLine.cxx
  vtkStreamPoints.cxx
  vtkStreamTracer.cxx
  vtkModifiedBSPTree.cxx
  vtkCellLocatorInterpolatedVelocityField.cxx
  )

set_source_files_properties(
  vtkAbstractInterpolatedVelocityField
  vtkStreamer
  ABSTRACT
  )

vtk_module_library(vtkFiltersTracers ${Module_SRCS})
