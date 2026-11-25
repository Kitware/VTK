set(classes
  vtkMPI4PyCommunicator)

vtk_module_add_module(VTK::ParallelMPI4Py
  CLASSES ${classes})

# The vendored mpi4py headers access PyTypeObject internals (tp_basicsize,
# tp_itemsize) directly, which the CPython Limited API forbids.  Opt this module
# out of the Limited API so those full-API members remain visible when VTK is
# configured with VTK_PYTHON_LIMITED_API.
vtk_module_definitions(VTK::ParallelMPI4Py
  PRIVATE
    VTK_PYTHON_NO_LIMITED_API)

vtk_add_test_mangling(VTK::ParallelMPI4Py)
