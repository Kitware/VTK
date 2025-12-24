## Add an abstract vtkCartesianGrid

A new abstract class, vtkCartesianGrid add a layer of abstraction
on both vtkImageData and vtkRectilinearGrid.

It is now possible to use a common API for both the classes.

No behavior were changed.

`vtkRectilinearGrid::ComputeStructuredCoordinates(double*, ...)` has been deprecated
in favor or `vtkRectilinearGrid::ComputeStructuredCoordinates(const double*, ...)`

`vtkImageData::SetExtent` is not `virtual` anymore.

`BuildImplicitStructures`, `BuildCells` and `BuildCellTypes` have been privatized, they are called as needed bt the vtkCartesianGrid.
`Extent`, `Dimensions` and `DataDescription` members have been privatized, use the accessors.
