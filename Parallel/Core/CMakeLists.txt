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
  vtkThreadedCallbackQueue)

set(templates
  vtkThreadedCallbackQueue.txx)

set(template_classes
  vtkThreadedTaskQueue)

include(vtkHashSource)
# Generate "vtkSocketCommunicatorHash.h".
vtk_hash_source(
  INPUT "vtkSocketCommunicator.cxx"
  NAME "vtkSocketCommunicatorHash"
  HEADER_OUTPUT hash_header)

vtk_module_add_module(VTK::ParallelCore
  CLASSES           ${classes}
  NOWRAP_HEADERS    vtkMultiProcessStreamSerialization.h
  TEMPLATES         ${templates}
  TEMPLATE_CLASSES  ${template_classes}
  PRIVATE_HEADERS   ${hash_header})
vtk_add_test_mangling(VTK::ParallelCore)
