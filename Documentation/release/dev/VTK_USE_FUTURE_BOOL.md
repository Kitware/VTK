## Added VTK_USE_FUTURE_BOOL configure-time variable

The codebase contains many variables typed as `int` that really should be `bool`. But changing them breaks backwards compatibility, and so a `vtkTypeBool` typedef was introduced which is defined to either `int` or `bool` depending on the new `VTK_USE_FUTURE_BOOL` configure-time variable. This allows for the piecemeal changing of many `int` variables to `vtkTypeBool`.
