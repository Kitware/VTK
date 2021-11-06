# vtkChemistryConfigure.h deprecated

The `vtkChemistryConfigure.h` header has been deprecated. This was only used to
provide information to VTK's test suite. As such, there is no replacement and
code can just remove the inclusion without losing any relevant information (as
it only worked with a VTK build tree and not an install tree).
