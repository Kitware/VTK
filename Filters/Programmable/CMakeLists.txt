set(classes
  vtkProgrammableAttributeDataFilter
  vtkProgrammableFilter
  vtkProgrammableGlyphFilter)

vtk_module_add_module(VTK::FiltersProgrammable
  CLASSES ${classes})
vtk_add_test_mangling(VTK::FiltersProgrammable)
