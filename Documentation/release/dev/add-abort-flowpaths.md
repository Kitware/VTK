# Add abort functionality to FlowPaths filters in VTK

FlowPaths filters now call `CheckAbort` to allow for
safe interruption during execution.
