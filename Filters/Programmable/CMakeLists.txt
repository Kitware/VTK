set(Module_SRCS
  vtkProgrammableAttributeDataFilter.cxx
  vtkProgrammableFilter.cxx
  vtkProgrammableGlyphFilter.cxx
  )

vtk_module_library(vtkFiltersProgrammable ${Module_SRCS})
