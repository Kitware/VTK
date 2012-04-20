set(Module_SRCS
  vtkCommunicator.cxx
  vtkDummyCommunicator.cxx
  vtkDummyController.cxx
  vtkMultiProcessController.cxx
  vtkMultiProcessStream.cxx
  vtkProcess.cxx
  vtkProcessGroup.cxx
  vtkSocketCommunicator.cxx
  vtkSocketController.cxx
  vtkSubCommunicator.cxx
  vtkSubGroup.cxx
  vtkFieldDataSerializer.cxx
  )

# Generate "vtkSocketCommunicatorHash.h".
add_custom_command(
  OUTPUT ${VTK_BINARY_DIR}/Parallel/Core/vtkSocketCommunicatorHash.h
  DEPENDS ${VTK_SOURCE_DIR}/Parallel/Core/vtkSocketCommunicator.cxx
          vtkHashSource
  COMMAND vtkHashSource
    ${VTK_SOURCE_DIR}/Parallel/Core/vtkSocketCommunicator.cxx
    vtkSocketCommunicatorHash vtkSocketCommunicatorHash.h
  )

list(APPEND Module_SRCS
  ${VTK_BINARY_DIR}/Parallel/Core/vtkSocketCommunicatorHash.h
  )

set_source_files_properties(
  vtkMultiProcessStream
  vtkSocketCommunicatorHash
  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkCommunicator
  vtkMultiProcessController
  vtkProcess
  ABSTRACT
  )

vtk_module_library(vtkParallelCore ${Module_SRCS})
