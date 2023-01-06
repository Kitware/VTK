# Add abort functionality to Modeling filters in VTK

Modeling filters now call `CheckAbort` to allow for
safe interruption during execution.
