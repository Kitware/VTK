# New vtkTableProbe filter

The `vtkTableProbe` filter probes a data array from a `vtkTable` (the "Source")
and adds it to any `vtkDataObject` (the "Input") dataset attribute.

The chosen columns of the Source can be seen as the axes of a N-dimension space.
Another column is the data of interest, that will be probed.
Each element in the Input can be seen as a random point in this space.
Thus the array names used as coordinates should be found in both the Source table and in the Input data object.

Then a probe is done in this space for each element of the Input, doing linear interpolation.
The output is a copy of Input, with the probed array added.
