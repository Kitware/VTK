## vtkResourceParser: high-performance formatted input over vtkResourceStream

The new `vtkResourceParser` class enables formatted input over `vtkResourceStream`s.
`vtkResourceParser` can parse strings, floats, integers and booleans from any vtkResourceStream
making it the perfect class to implement `vtkResourceStream` support in both existing and new
readers. Most of `std::istream` common features have equivalent methods in `vtkResourceParser`, making the migration of most existing readers trivial.
