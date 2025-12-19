## Faster vtkDataAssembly node traversal.

Unify implementation of `IsNodeNameReserved` and add short-circuiting logic to optimize for typical usage. Improves
performance of `GetChild` and dependant methods by about 50%.
