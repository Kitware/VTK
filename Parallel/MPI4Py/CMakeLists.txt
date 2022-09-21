set(classes
  vtkMPI4PyCommunicator)

vtk_module_add_module(VTK::ParallelMPI4Py
  CLASSES ${classes})
vtk_add_test_mangling(VTK::ParallelMPI4Py)
