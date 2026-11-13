## vtkAMRInterfaceFilter: Distributed AMR and ghost layers

`vtkAMRInterfaceFilter` now builds the interfaces between levels of a distributed
`vtkOverlappingAMR`. Each process exchanges the grids that touch its own with the processes that
own them, through the new `Controller`, which defaults to the global controller. Before, an
interface with a finer grid owned by another process was not built, which left gaps between levels
in contours computed in parallel.

The filter also removes the layers of duplicate ghost cells around image grids before building the
interfaces, so it can follow `vtkGhostCellsGenerator`.
