# Add abort functionality to AMR filters in VTK

AMR filters now call `CheckAbort` to allow for
safe interruption during execution.
`vtkAMRUtilities::BlankCells` will be skipped
if a filter's `CheckAbort` returns true.
