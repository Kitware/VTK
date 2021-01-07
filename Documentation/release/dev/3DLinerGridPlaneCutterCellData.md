## vtk3DLinearGridPlaneCutter handle cell data

The `vtk3DLinearGridPlaneCutter` has been updated to also handle cell data.
Each triangle of the output data set contains the attributes of the cell it
originated from.
Using this class should be faster than using the `vtkUnstructuredGridCutter`
or the `vtkDataSetCutter` and should avoid some small projection error.
The `vtkCutter` has also been updated to benefit from these changes.
