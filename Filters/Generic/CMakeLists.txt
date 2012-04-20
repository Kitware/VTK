set(Module_SRCS
  vtkGenericClip.cxx
  vtkGenericContourFilter.cxx
  vtkGenericCutter.cxx
  vtkGenericDataSetTessellator.cxx
  vtkGenericGeometryFilter.cxx
  vtkGenericGlyph3DFilter.cxx
  vtkGenericOutlineFilter.cxx
  vtkGenericProbeFilter.cxx
  vtkGenericStreamTracer.cxx
  )

vtk_module_library(vtkFiltersGeneric ${Module_SRCS})
