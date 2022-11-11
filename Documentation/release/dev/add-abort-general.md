# Add abort functionality to General filters in VTK

General filters now call `CheckAbort` to allow for
safe interruption during execution.
