## `VTK_USE_MPI`/`VTK_USE_TK` are less aggresive

The configure option `VTK_USE_MPI` and `VTK_USE_TK` no longer forces their
respective Groups on. They will now either change the default to `WANT` or use
the specified value for `VTK_GROUP_ENABLE_{MPI,Tk}`.

There is now an explicit error when either `VTK_USE_{MPI,TK}` is `ON` but the
respective group is configured as `NO`.
