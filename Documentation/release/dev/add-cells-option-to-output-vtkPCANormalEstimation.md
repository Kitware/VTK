## Add option to add cells to the output of vtkPCANormalEstimation

The vtkPCANormalEstimation support now adding cells to the output `vtkPolyData` to enable the visualization on paraview, the user can set the variable CellGenerationMode to 0: No cells will be added (default option), 1: A single cell encompassing the entire `vtkPolyData` and 2: A cell for each point
