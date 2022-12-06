# Add abort functionality to Geometry filters in VTK

Geometry filters now call `CheckAbort` to allow for
safe interruption during execution.
