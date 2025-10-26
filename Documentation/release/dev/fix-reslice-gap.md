# Fix tiny gap in some vtkImageReslice code paths

When interpolating between slices with vtkImageReslice, there was a
tolerance issue that could cause black slices in rare situations when
the pipeline was streaming slice-by-slice, that is, when vtkImageReslice
was connected upstream and downstream via SetInputConnection() rather
than SetInputData(), and when the upstream was not manually updated by
e.g. calls to Update() or UpdateWholeExtent().  The black slice could
occur when the vtkImageReslice transform caused it to produce a single
slice nearly halfway between two of the input slices, specifically at
a fractional distance of 0.499993 to 0.499999 but not at exactly 0.5.
This bug has now been fixed.
