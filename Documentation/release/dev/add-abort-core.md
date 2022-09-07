# Add abort functionality to Core filters in VTK

Core filters now call `CheckAbort` to allow for
safe interruption during execution.
