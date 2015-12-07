set(Module_SRCS
  vtkAbstractInterpolatedVelocityField.cxx
  vtkAMRInterpolatedVelocityField.cxx
  vtkCompositeInterpolatedVelocityField.cxx
  vtkInterpolatedVelocityField.cxx
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
  vtkParticleTracerBase
  ABSTRACT
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
