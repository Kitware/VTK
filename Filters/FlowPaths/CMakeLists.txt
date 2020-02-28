set(classes
  vtkAbstractInterpolatedVelocityField
  vtkAMRInterpolatedVelocityField
  vtkCachingInterpolatedVelocityField
  vtkCellLocatorInterpolatedVelocityField
  vtkCompositeInterpolatedVelocityField
  vtkEvenlySpacedStreamlines2D
  vtkInterpolatedVelocityField
  vtkLagrangianBasicIntegrationModel
  vtkLagrangianMatidaIntegrationModel
  vtkLagrangianParticle
  vtkLagrangianParticleTracker
  vtkModifiedBSPTree
  vtkParallelVectors
  vtkParticlePathFilter
  vtkParticleTracer
  vtkParticleTracerBase
  vtkStreaklineFilter
  vtkStreamTracer
  vtkTemporalInterpolatedVelocityField
  vtkVortexCore)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND classes vtkTemporalStreamTracer)
endif()
vtk_module_add_module(VTK::FiltersFlowPaths
  CLASSES ${classes})
