set(Module_SRCS
  vtkColorSeries.cxx
  vtkNamedColors.cxx
)

#set_source_files_properties(
#  ABSTRACT
#  WRAP_EXCLUDE
#  PROPERTIES WRAP_SPECIAL 1
#  PROPERTIES WRAP_HEADER 1
#)

vtk_module_library(vtkCommonColor ${Module_SRCS})
