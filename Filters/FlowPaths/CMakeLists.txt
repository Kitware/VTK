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
  vtkStreamSurface
  vtkStreamTracer
  vtkTemporalInterpolatedVelocityField
  vtkTemporalStreamTracer
  vtkVectorFieldTopology
  vtkVortexCore)

vtk_module_add_module(VTK::FiltersFlowPaths
  CLASSES ${classes})
