## Move `vtkAttributeDataToTableFilter` from ParaView to VTK

The `vtkAttributeDataToTableFilter` is now available in VTK. It used to be in the `VTKExtensions` of the ParaView repository. It serves to turn a data object into a table by shallow copying its attributes into row data.

Seeing as the `vtkAttributeDataToTableFilter` does the same thing as `vtkDataObjectToTable` filter with some added functionality, **the `vtkDataObjectToTable` filter has been deprecated**.

Support for `vtkHyperTreeGrid` has also been trivially added to the `vtkAttributeDataToTableFilter`.
