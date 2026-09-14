#  vtkGhostCellsGenerator: Fix duplication of points with globalIds in 3+ partitions in UnstructuredGrid

The vtkGhostCellsGenerator filter for unstructured grid no longer duplicates ghost points sent by more than 1 process.
