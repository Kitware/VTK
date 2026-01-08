## Macro VTK_PYTHON_HAS_GIL introduced in vtkPython.h

The macro `VTK_PYTHON_HAS_GIL` has been added to `vtkPython.h` with the aim to
consolidate the state of effectively no GIL. Before this change, this was
indirectly inferred by the value of: `VTK_NO_PYTHON_THREADS`,
`VTK_PYTHON_FULL_THREADSAFE`, and the CPython macro `Py_GIL_DISABLED`.
Note that this macro forces `VTK_PYTHON_FULL_THREADSAFE=OFF` when
`VTK_PYTHON_HAS_GIL=OFF`.

This macro is available in `vtkPython.h`.
