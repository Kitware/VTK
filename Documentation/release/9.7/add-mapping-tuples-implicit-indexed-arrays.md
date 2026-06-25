# Add the possibility to map tuples instead of values in implicit indexed arrays

It is now possible to choose in `vtkIndexedArray` if we want to map the id array with tuples instead of values.
If so, the value index will be retrieved using the number of components in the base array. By default, it maps values.
