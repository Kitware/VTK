## Addition of `PassThroughCellIds` property in `vtkHyperTreeGridGeometry` filter

The `vtkHyperTreeGridGeometry` filter now provides and option `PassThroughCellIds` (default `false`) to pass through original cell IDs from the input `vtkHyperTreeGrid` to the output `vtkPolyData`. One can also name the array using the `OriginalCellIdArrayName` string property.

This option, often found in geometry filters, allows one to make the resulting `vtkPolyData` compatible with the `vtkHardwareSelector` for making selections on the original `vtkHyperTreeGrid`. It can also help for debugging purposes as well as general additional visualization data when mapping the surface to color.
