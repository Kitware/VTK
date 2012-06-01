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
  vtkTemporalStreamTracer.cxx
  vtkParticleTracerBase.cxx
  vtkParticleTracer.cxx
  vtkStreaklineFilter.cxx
  vtkParticlePathFilter.cxx
  vtkTemporalInterpolatedVelocityField.cxx
  vtkCachingInterpolatedVelocityField.cxx
  )

set_source_files_properties(
  vtkAbstractInterpolatedVelocityField
  vtkCompositeInterpolatedVelocityField.cxx
  vtkStreamer
  vtkParticleTracerBase
  ABSTRACT
  )

vtk_module_library(vtkFiltersFlowPaths ${Module_SRCS})
