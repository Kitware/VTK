set(Module_SRCS
  vtkXdmf3ArraySelection.cxx
  vtkXdmf3DataSet.cxx
  vtkXdmf3Reader.cxx
  vtkXdmf3Writer.cxx
  )

set_source_files_properties(
  vtkXdmf3ArraySelection
  vtkXdmf3DataSet
  WRAP_EXCLUDE
)

include_directories(${XdmfCore_INCLUDE_DIRS} ${XdmfDSM_INCLUDE_DIRS})
vtk_module_library(vtkIOXdmf3 ${Module_SRCS})
