set(classes
  vtkCommunicator
  vtkDummyCommunicator
  vtkDummyController
  vtkFieldDataSerializer
  vtkMultiProcessController
  vtkMultiProcessStream
  vtkPDirectory
  vtkProcess
  vtkProcessGroup
  vtkPSystemTools
  vtkSocketCommunicator
  vtkSocketController
  vtkSubCommunicator
  vtkSubGroup
)

include(vtkHashSource)
# Generate "vtkSocketCommunicatorHash.h".
vtk_hash_source(
  INPUT "vtkSocketCommunicator.cxx"
  NAME "vtkSocketCommunicatorHash"
  HEADER_OUTPUT hash_header)

vtk_module_add_module(VTK::ParallelCore
  CLASSES           ${classes}
  NOWRAP_HEADERS    vtkMultiProcessStreamSerialization.h
  PRIVATE_HEADERS   ${hash_header}
  # This generated header doesn't contain anything with copyright.
  SPDX_SKIP_REGEX   "vtkSocketCommunicatorHash")
vtk_add_test_mangling(VTK::ParallelCore)
