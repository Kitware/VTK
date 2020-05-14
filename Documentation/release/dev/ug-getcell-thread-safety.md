## Fix thread safety issue in vtkUnstructuredGrid::GetCell

The `vtkUnstructuredGrid::GetCell` method that takes a `vtkGenericCell` is
supposed to be thread safe. However, there was an issue where `GetCell`
called a method that was unsafe under certain conditions.
The problem was thta GetCell was calling the version of
`vtkCellArray::GetCellAtId` that returned the pointer to the internal
array of indices. That was thread safe if `vtkCellArray` happened to use a
simple array of type `vtkIdType` to store the indices. However, if the
array was of a different type, it copied the values to an array stored
in `vtkCellArray` and a pointer to that is returned. This is very much not
thread safe.

The problem is fixed by using a different version of `GetCellAtId` that
takes in the `vtkIdList` of the generic cell. Since the connectivity has to
be copied to that array anyway, there is no real performance penalty for
this.
