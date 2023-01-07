## Compile-time inheritance tree

VTK now provides a way to iterate over a class and
all its ancestor types (as long as they inherit
`vtkObjectBase` and use the `vtkTypeMacro()` to
define `Superclass` type-aliases).
The `vtk::ParentClasses<T>::enumerate()` function
will invoke a functor you pass on `T` and each
superclass of `T`.

This is used by a new `vtk::Inheritance<T>()`
function that inserts the name of each class inherited
by `T` into a container you pass to it.

See the `TestInherits` in `Common/Core/Testing/Cxx` for
an example of its usage.
