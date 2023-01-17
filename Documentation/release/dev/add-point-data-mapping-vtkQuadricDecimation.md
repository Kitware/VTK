## `vtkQuadricDecimation` can now map point data to its decimated output

You can now map point data from the input of the `vtkQuadricDecimation` filter to its decimated output by setting the new boolean property `MapPointData` to `true`. The output values are interpolated on the edges before they are collapsed.
