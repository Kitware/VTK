## Fix vtkCellValidator for vtkVoxel and vtkPixel

The `vtkCellValidator` filter now properly access point coordinates for `vtkVoxel` and `vtkPixel`
to position the `IntersectingEdges` flag.
In the previous version, a wrong point Id was used leading to potential crash.
