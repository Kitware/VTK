## Fix slowdown when retrieving many arrays in Python

Retrieving many data arrays in Python, for example calling
`GetAbstractArray()` on each of thousands of arrays in a `vtkCellData`,
is fast again. Since VTK 9.7 the time taken grew quadratically with the
number of arrays.
