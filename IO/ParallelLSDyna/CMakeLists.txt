set(Module_SRCS
  vtkPLSDynaReader.cxx
  )

include_directories(
  ${vtkIOLSDyna_SOURCE_DIR}/private
  ${vtkIOLSDyna_BINARY_DIR}/private
  )

vtk_module_library(vtkIOParallelLSDyna ${Module_SRCS})
