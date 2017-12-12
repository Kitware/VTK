set(Module_SRCS
  vtkPLY.cxx
  vtkPLYReader.cxx
  vtkPLYWriter.cxx
)

vtk_module_library(vtkIOPLY ${Module_SRCS})
