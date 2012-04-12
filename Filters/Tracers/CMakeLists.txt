set(Module_SRCS
  vtkAbstractInterpolatedVelocityField.cxx
  vtkAMRInterpolatedVelocityField.cxx
  vtkCompositeInterpolatedVelocityField.cxx
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
  vtkCompositeInterpolatedVelocityField.cxx
  vtkStreamer
  ABSTRACT
  )

vtk_module_library(vtkFiltersTracers ${Module_SRCS})
