## Fix vtkGhostCellsGenerator with GenerateGlobalIds

The output of `vtkGhostCellsGenerator` is now coherent whether the `GenerateGlobalIds` option is toggled or not.
Point id generation now uses a small tolerance to merge points close by, to match the behavior of point locators used when
the ghost cells generator does not have global ids to match points.
