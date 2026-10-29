## vtkGhostCellsGenerator: Ghosts of structured blocks whose faces do not line up

`vtkGhostCellsGenerator` now generates correct ghosts for `vtkImageData`, `vtkRectilinearGrid` and
`vtkStructuredGrid` partitions whose faces do not line up, where a block borders several neighbors
along one face, as on the levels of an AMR hierarchy. Before, the ghost extents of such blocks
were wrong, and rectilinear grids could hang under MPI.

Ghost generation also keeps the `REFINEDCELL`, `HIGHCONNECTIVITYCELL`, `LOWCONNECTIVITYCELL` and
`EXTERIORCELL` values of an input cell ghost array, such as the `REFINEDCELL` blanking of AMR
datasets. Before, the input ghost array was discarded when it held no ghosts, and those values were
lost.
