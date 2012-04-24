set(Module_SRCS
  vtkPLY.cxx
  vtkPLYReader.cxx
  vtkPLYWriter.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkPLY
  WRAP_EXCLUDE
)

vtk_module_library(vtkIOPLY ${Module_SRCS})
