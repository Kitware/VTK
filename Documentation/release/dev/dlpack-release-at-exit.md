## DLPack: no crash at exit while an imported GPU tensor is in use

A device tensor imported with `vtkmodules.util.dlpack_support` no longer
crashes the interpreter at exit when its VTK array is still alive -- held
through a tensor exported back to Python, say. VTK deletes such arrays from a
`Py_AtExit` hook, after the interpreter has finalized, and the tensor used to
be released by a ctypes callback that then entered the dead interpreter.

`vtkmodules.vtkCommonCore` has a C release function for
`vtkMemoryDescriptor.SetReleaseAddress`, `release_callable_address()`, that
calls a Python callable only while the interpreter is up and otherwise leaks
the hold. `dlpack_support` releases imported tensors through it.
