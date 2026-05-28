# Add use of implicit arrays in some filters

Some filters now uses implicit arrays to minimize memory consumption and to speed up the execution of the filter. However, it comes with the cost of a slightly slower access to the array elements.

The filters concerned by this change are :
- **vtkAppendCompositeDataLeaves**
- **vtkAppendDataSets**
- **vtkAppendFilter**
- **vtkAppendPolyData**
- **vtkGhostCellsGenerator**

This is optionnal and set to false by default, use `UseImplicitArrayOn` to enable it.
