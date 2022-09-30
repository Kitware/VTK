# Add abort functionality to Extraction filters in VTK

Extraction filters now call `CheckAbort` to allow for
safe interruption during execution.
