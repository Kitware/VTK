## vtkXYZMolReader2 handles invalid files gracefully

`vtkXYZMolReader2` no longer crashes when given a malformed XYZ file. The
reader now guards against an empty set of timesteps when parsing fails,
returning an error instead of indexing out of bounds.

The reader also gains `CanReadFile()` overloads, taking either a file path
or a `vtkResourceStream`, that report whether the content begins with a
positive atom count. Use them to check a file before reading it.
