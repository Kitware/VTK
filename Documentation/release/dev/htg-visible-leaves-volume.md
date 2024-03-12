## Hyper Tree Grid Compute Visible Leaves Volume filter

A new HTG utility filter for HTG has been added : `vtkHyperTreeGridComputeVisibleLeavesVolume`.
This filter creates 2 new cell fields using implicit arrays under the hood:
- `vtkValidCell` has a (double) value of 1.0 for visible (non ghost, non masked) leaf cells, and 0.0 for the others.
- `vtkVolume`'s value corresponds to the volume of the cell.

`vtkValidCell` can allow, with `vtkVolume`, to compute volume aggregations over the HTG.
