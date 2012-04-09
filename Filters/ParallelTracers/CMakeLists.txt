set(Module_SRCS
  vtkTemporalStreamTracer.cxx
  vtkPStreamTracer.cxx
  vtkDistributedStreamTracer.cxx
  )

set_source_files_properties(
  vtkPStreamTracer
  vtkDistributedStreamTracer
  WRAP_EXCLUDE
  )

vtk_module_library(vtkFiltersParallelTracers ${Module_SRCS})
