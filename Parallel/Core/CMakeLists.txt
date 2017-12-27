set(Module_SRCS
  vtkCommunicator.cxx
  vtkDummyCommunicator.cxx
  vtkDummyController.cxx
  vtkMultiProcessController.cxx
  vtkMultiProcessStream.cxx
  vtkPDirectory.cxx
  vtkProcess.cxx
  vtkProcessGroup.cxx
  vtkPSystemTools.cxx
  vtkSocketCommunicator.cxx
  vtkSocketController.cxx
  vtkSubCommunicator.cxx
  vtkSubGroup.cxx
  vtkFieldDataSerializer.cxx
  )

# Generate "vtkSocketCommunicatorHash.h".
include(vtkHashSource)
vtk_hash_source(
  INPUT "vtkSocketCommunicator.cxx"
  NAME "vtkSocketCommunicatorHash"
  HEADER_OUTPUT hash_header)

list(APPEND Module_SRCS ${hash_header})
vtk_module_library(vtkParallelCore ${Module_SRCS})
