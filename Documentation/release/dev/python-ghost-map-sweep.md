## Fix slowdown when retrieving many arrays in Python

Retrieving many data arrays in Python, for example calling
`GetAbstractArray()` on each of thousands of arrays in a `vtkCellData`,
is fast again and no longer slows down quadratically with the number of
arrays.

Since VTK 9.7, the Python classes for arrays, field data and datasets set
default attributes on every wrapper, so VTK saved each discarded wrapper's
state in a ghost map, and adding each ghost scanned the whole map for
deleted objects. The defaults are now class attributes, so a wrapper that
has no state of its own is no longer saved. When a wrapper does have state,
adding its ghost no longer scans the map: ghosts of `vtkObject`s are removed
when their object is deleted, and only the ghosts that cannot be removed
that way are checked.
