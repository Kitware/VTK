## Add double precision merge points compatibility to `vtkMergeCells`

The filter `vtkMergeCells` has been enhanced by adding the possibility of merging points using a double precision tolerance. The previous implementation was only compatible with single precision tolerances. You can now use this filter with meshes with distance between points lower than single precision accuracy.
