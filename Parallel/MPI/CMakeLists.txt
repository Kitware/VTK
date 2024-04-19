set(classes
  vtkMPICommunicator
  vtkMPIController
  vtkMPIUtilities)

set(nowrap_headers
  vtkMPI.h)

vtk_module_add_module(VTK::ParallelMPI
  CLASSES ${classes}
  NOWRAP_HEADERS ${nowrap_headers})
vtk_add_test_mangling(VTK::ParallelMPI)
