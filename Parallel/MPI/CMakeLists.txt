set(classes
  vtkMPICommunicator
  vtkMPIController
  vtkMPIUtilities)

set(headers
  vtkMPI.h)

vtk_module_add_module(VTK::ParallelMPI
  CLASSES ${classes}
  HEADERS ${headers})
