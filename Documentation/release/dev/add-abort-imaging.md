# Add abort functionality to Imaging filters in VTK

Imaging filters now call `CheckAbort` to allow for
safe interruption during execution.
