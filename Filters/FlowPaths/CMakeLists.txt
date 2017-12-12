set(Module_SRCS
  vtkAbstractInterpolatedVelocityField.cxx
  vtkAMRInterpolatedVelocityField.cxx
  vtkCachingInterpolatedVelocityField.cxx
  vtkCellLocatorInterpolatedVelocityField.cxx
  vtkCompositeInterpolatedVelocityField.cxx
  vtkEvenlySpacedStreamlines2D.cxx
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

vtk_module_library(vtkFiltersFlowPaths ${Module_SRCS})
