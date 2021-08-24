## Introduce new vtkArrayRename filter

VTK now provides a filter to rename data arrays.
You should manually specify the new names.
New names should be unique inside attributes type array list.

The arrays are Shallow Copied, so there is no copy of the actual data,
excepted for `vtkStringArrays`, which has no `ShallowCopy` method and
thus are Deep Copied.
