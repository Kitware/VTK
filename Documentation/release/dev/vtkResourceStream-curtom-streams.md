## vtkResourceStream: an interface for custom streams in VTK

The new `vtkResourceStream` class is an interface that aims to replace standard istreams.
`vtkResourceStream` aims to be used in API that required a data source (e.g. readers) to enable custom streams support.
For now 2 implementation are provided, `vtkFileResourceStream` and `vtkMemoryResourceStream`, for file and memory input respectively.
