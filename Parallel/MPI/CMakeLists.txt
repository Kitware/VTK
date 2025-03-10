set(classes
  vtkMPICommunicator
  vtkMPIController
)

set(nowrap_headers
  vtkMPI.h
  vtkMPIUtilities.h)

vtk_module_add_module(VTK::ParallelMPI
  CLASSES ${classes}
  NOWRAP_HEADERS ${nowrap_headers})
vtk_add_test_mangling(VTK::ParallelMPI)
