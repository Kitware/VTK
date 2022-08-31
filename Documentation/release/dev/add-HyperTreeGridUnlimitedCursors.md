# Add vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursors & vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor

The `vtkHyperTreeGrid` class has a new kind of cursors, called
**unlimited**. These cursors allows to traverse the tree and go deeper than
a leaf. This allows for example to simulate a uniform resolution near a specific
cell of the tree.

The depth limited should still work, but for now masks are not supported.
