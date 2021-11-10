# Deprecation in vtkCellTypes

Usage of `LocationArray` in `vtkCellTypes` is deprecated. This is an old legacy feature that was
used by `vtkUnstructuredGrid`. Currently, this class is used to store a list of distinct cell types
present in a `vtkUnstructuredGrid`, making `LocationArray` irrelevant.
In the long run, `vtkCellTypes` will become a static method only class. So it is advised to not use
instances of this class anymore. Using static methods is fine.

A new more direct interface to the array listing distinct cell types is implemented
in `vtkUnstructuredGrid` (see release notes about deprecation in `vtkUnstructuredGrid`).
