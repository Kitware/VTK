# Add abort functionality to SMP filters in VTK

SMP filters now call `CheckAbort` to allow for
safe interruption during execution.

In order to prevent multiple calls to
`CheckAbort`, `vtkSMPTools` has a new function
to create a cheap single scope for all
implementations. The function `GetSingleThread`
returns true if the thread is the single thread.
Otherwise the function returns false.
