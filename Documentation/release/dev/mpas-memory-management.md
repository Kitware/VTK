## Fixed potential memory management in vtkMPASReader

`vtkMPASReader` allocates several temporary arrays while reading data. Under
normal conditions, these get deleted, but there is the potential for them to
leak data. Update the code to group arrays in a structure that ensures data gets
cleared when data loading finishes.

As part of this change, some of the protected members of `vtkMPASReader`
have been made private. This is necessary for changes in the implementation.
