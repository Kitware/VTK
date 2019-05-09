set(classes
  vtkPLagrangianParticleTracker
  vtkPParticlePathFilter
  vtkPParticleTracer
  vtkPParticleTracerBase
  vtkPStreaklineFilter
  vtkPStreamTracer)

# Overrides for object factory.
set(parallel_overrides
  LagrangianParticleTracker
  StreamTracer
  ParticleTracer
  ParticlePathFilter
  StreaklineFilter)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND classes vtkPTemporalStreamTracer)
  list(APPEND parallel_overrides TemporalStreamTracer)
endif()

foreach (parallel_override IN LISTS parallel_overrides)
  vtk_object_factory_declare(
    BASE      vtk${parallel_override}
    OVERRIDE  vtkP${parallel_override})
endforeach ()

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKFILTERSPARALLELFLOWPATHS_EXPORT")

vtk_module_add_module(VTK::FiltersParallelFlowPaths
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  HEADERS ${vtk_object_factory_header})
