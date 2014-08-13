set(Module_SRCS
  vtkXdmf3ArrayKeeper.cxx
  vtkXdmf3ArraySelection.cxx
  vtkXdmf3DataSet.cxx
  vtkXdmf3HeavyDataHandler.cxx
  vtkXdmf3LightDataHandler.cxx
  vtkXdmf3Reader.cxx
  vtkXdmf3SILBuilder.cxx
  vtkXdmf3Writer.cxx
  )

set_source_files_properties(
  vtkXdmf3ArrayKeeper
  vtkXdmf3ArraySelection
  vtkXdmf3DataSet
  vtkXdmf3HeavyDataHandler
  vtkXdmf3LightDataHandler
  vtkXdmf3SILBuilder
  WRAP_EXCLUDE
)

include_directories(${XdmfCore_INCLUDE_DIRS} ${XdmfDSM_INCLUDE_DIRS})
vtk_module_library(vtkIOXdmf3 ${Module_SRCS})
