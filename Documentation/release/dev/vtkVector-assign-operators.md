## vtkVector assignment operators

The `vtkVector` `+=` and `-=` operators have been changed to return a
`vtkVector&` rather than a `vtkVector`. This may break some callers, but these
were already not behaving as expected because the returned value was an
uninitialized vector, not the result of the operation.
