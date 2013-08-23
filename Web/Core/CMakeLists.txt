set (Module_SRCS
  vtkDataEncoder.cxx
  vtkWebApplication.cxx
  vtkWebInteractionEvent.cxx
  vtkWebUtilities.cxx
  vtkObjectIdMap.cxx
)

vtk_module_library(${vtk-module}
  ${Module_SRCS})
