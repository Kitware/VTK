### vtkDataSetSurfaceFilter and structured datasets with blanking

vtkDataSetSurfaceFilter now supports extracting surfaces from all types of
structured datasets (`vtkImageData`, `vtkStructuredGrid`,
`vtkRectilinearGrid` and subclasses) when they have blanked cells marked
using ghost arrray.

The filter also supports a new fast mode, which can generate this surface
quickly by only considering the outer-most surfaces and not worrying about
external-faces internal to the outer shell. This may be suitable when the surface is
being extracted simply for rendering purposes as it's much faster and generally
will produce results that are adequate for viewing.
