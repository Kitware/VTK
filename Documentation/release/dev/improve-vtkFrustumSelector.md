## vtkFrustumSelector: Improvements

vtkFrustumSelector's performance has been improved by copying the frustum planes only once instead for each cellId, and
also it first extracts the cell bounds, and only if they are not sufficient, it extracts the cell.

Additionally, vtkBoundingBox has a new ComputeBounds method that computes the bounds of a set of points given specific
ids. This new function is used for the GetCellBounds function of vtkUnstructuredGrid, vtkPolyData,
vtkExplicitStructuredGrid.

Finally, vtkPolyData's ComputeCellsBounds has been improved by removing the unnecessary usages of atomics and using the
thread safe version of GetCellPoints instead of a cell iterator.
