set(Module_SRCS
  vtkColorSeries.cxx
  vtkNamedColors.cxx
)

#set_source_files_properties(
#  ABSTRACT
#  WRAP_EXCLUDE
#)

vtk_module_library(vtkCommonColor ${Module_SRCS})
