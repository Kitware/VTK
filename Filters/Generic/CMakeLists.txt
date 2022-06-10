set(classes
  vtkGenericClip
  vtkGenericContourFilter
  vtkGenericCutter
  vtkGenericDataSetTessellator
  vtkGenericGeometryFilter
  vtkGenericGlyph3DFilter
  vtkGenericOutlineFilter
  vtkGenericProbeFilter
  vtkGenericStreamTracer)

vtk_module_add_module(VTK::FiltersGeneric
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersGeneric)
