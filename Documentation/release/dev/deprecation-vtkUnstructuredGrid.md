# Deprecation in vtkUnstructuredGrid

The method `GetCellTypes` is deprecated in `vtkUnstructuredGrid` (see release notes about
deprecation in `vtkCellTypes`). A new method is proposed to use instead, directly returning the
array listing the distinct cells types. This method is named `GetDistinctCellTypesArray`, and
returns a `vtkUnsignedCharArray*`. This method should be used instead of `GetCellTypes` to access
this information.
