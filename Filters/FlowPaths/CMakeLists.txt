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

if(NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_SRCS
    vtkDashedStreamLine.cxx
    vtkStreamLine.cxx
    vtkStreamPoints.cxx
    vtkStreamer.cxx)

  set_source_files_properties(
    vtkStreamer
    ABSTRACT
    )
endif ()

vtk_module_library(vtkFiltersFlowPaths ${Module_SRCS})
