## Fix vtkBandedPolyDataContourFilter scalars

You can now select scalars used by `vtkBandedPolyDataContourFilter` using the `SetInputArrayToProcess` method instead
of changing the scalars on the input data.
