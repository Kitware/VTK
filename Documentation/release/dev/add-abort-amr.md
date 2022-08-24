# Add abort functionality to AMR filters in VTK

AMR filters now call `CheckAbort` to allow for
safe interruption during execution.
`vtkAMRUtilities::BlankCells` will be skippped
if a filter's `CheckAbort` returns true.
