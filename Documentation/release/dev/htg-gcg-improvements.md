## Hyper Tree Grid Ghost Cells support

Hyper Tree Grid Ghost Cell Generator now supports multi-components array to be transferred between MPI ranks.

A new filter `vtkHyperTreeGridExtractGhostCells` has been created to extract ghost cells for a HTG,
similarly to what `vtkExtractGhostCells` does for other data types.

The class has been reorganized internally, providing more precise debugging log and progress feedback.
Mask support has been improved and optimized: refined masked cells will not send their full tree decomposition anymore.
