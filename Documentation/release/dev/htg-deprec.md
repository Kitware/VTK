# Deprecate unused vtkHyperTree(Grid) methods and options

Deprecate unimplemented method `vtkHyperTree::Freeze`. Deprecate `vtkHyperTreeGrid::Squeeze` that had no effect unless `vtkHyperTree::Freeze` was implemented. Remove unused `vtkHyperTreeGrid::ModeSqueeze` and `vtkHyperTreeGrid::FreezeState` members.
