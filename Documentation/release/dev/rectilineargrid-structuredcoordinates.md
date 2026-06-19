## Fix a bug in vtkRectilinearGrid::ComputeStructuredCoordinates

The boundaries points of the grid are now correctly marked as part of the grid in
the `ComputeStructuredCoordinates`, so the computation continue and return the
correct coordinates instead of aborting with a default return value.
