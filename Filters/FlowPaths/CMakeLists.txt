set(Module_SRCS
  vtkAbstractInterpolatedVelocityField.cxx
  vtkAMRInterpolatedVelocityField.cxx
  vtkCachingInterpolatedVelocityField.cxx
  vtkCellLocatorInterpolatedVelocityField.cxx
  vtkCompositeInterpolatedVelocityField.cxx
  vtkInterpolatedVelocityField.cxx
  vtkLagrangianParticleTracker.cxx
  vtkLagrangianBasicIntegrationModel.cxx
  vtkLagrangianParticle.cxx
  vtkLagrangianMatidaIntegrationModel.cxx
  vtkModifiedBSPTree.cxx
  vtkParticlePathFilter.cxx
  vtkParticleTracer.cxx
  vtkParticleTracerBase.cxx
  vtkStreaklineFilter.cxx
  vtkStreamTracer.cxx
  vtkTemporalInterpolatedVelocityField.cxx
  vtkTemporalStreamTracer.cxx
  )

set_source_files_properties(
  vtkAbstractInterpolatedVelocityField
  vtkCompositeInterpolatedVelocityField.cxx
  vtkParticleTracerBase
  ABSTRACT
  )

set_source_files_properties(
  vtkLagrangianParticle.cxx
  WRAP_EXCLUDE
  )

vtk_module_library(vtkFiltersFlowPaths ${Module_SRCS})
