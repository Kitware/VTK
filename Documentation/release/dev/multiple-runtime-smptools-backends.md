# Add runtime SMPTools backends selection

Change SMPTools architecture to allow compiling multiple backends (Sequential, TBB, STDThread, OpenMP) in the same build and change them on runtime.

The `VTK_SMP_IMPLEMENTATION_TYPE` cmake option set which SMPTools will be implemented by default.
The backend can be then changed at runtime with the environment variable `VTK_SMP_BACKEND_IN_USE`.
Note that the desired backend must have his option `VTK_SMP_ENABLE_<backend_name>` set to `ON` to be able to use it.
