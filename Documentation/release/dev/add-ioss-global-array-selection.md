## vtkIOSSReader: Replace SetReadGlobalFields with vtkDataArraySelection* GlobalFieldSelection()

`vtkIOSSReader` used to have a method `SetReadGlobalFields` to control if the global fields would be reader or not. This
method has been deprecated in favor of a more flexible approach using `vtkDataArraySelection* GlobalFieldSelection()` to
allow users to select specific global fields to be read. This is particularly useful when dealing with datasets with a
large number of global fields, as it allows users to read only the fields they are interested in, improving performance
and reducing memory usage.
