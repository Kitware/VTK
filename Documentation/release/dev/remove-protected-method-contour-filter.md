## Remove contour filter's protected static method

Due to the rework of the `vtkContourFilter` to better support execution abort, some class members visibility changed.

The progress event callback method is now part of the anonymous namespace of `vtkContourFilter.cxx`, and therefore removed from the filter class header. Moreover, the progress event callback is now a private member of the class instead of protected.
