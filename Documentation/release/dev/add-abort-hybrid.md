# Add abort functionality to Hybrid filters in VTK

Hybrid filters now call `CheckAbort` to allow for
safe interruption during execution.
