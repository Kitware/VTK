## Data set mapper serialization no longer emits an empty polydata

When a `vtkDataSetMapper` has a `vtkPolyData` input, serialization now updates
the input algorithm before serializing the data set. Previously, if the
producer of the input had never been executed, the `ExtractedPolyData` entry in
the mapper's state held an empty polydata, so the deserialized scene rendered
nothing.
