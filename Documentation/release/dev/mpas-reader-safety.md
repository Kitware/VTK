## Improved the safety of vtkMPASReader code.

A recent improvement to `vtkMPASReader` is the ability to read either the dual
grid or primary grid. Supporting both these creates some confusing semantic
switching of the point and cell information. Some corrections on the internal
behavior of the implementation makes the code more robust.
