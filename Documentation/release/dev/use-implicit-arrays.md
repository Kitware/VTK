## Use implicit arrays in more filters

Some filters now benefit from implementations using implicit arrays
in order to save array storage memory.

In particular:
- `vtkGenerateProcessIds`, `vtkBlockIdScalars` and `vtkOverlappingAMRLevelIdScalars` use a `vtkConstantArray`.
- `vtkCountFaces` and `vtkCountVertices` optionally compute number of faces and vertices values on-demand instead of storing them. This option saves memory but can slow down array element access for structured datasets, so it's disabled by default.

As a consequence of this change, the output arrays of these filters cannot be cast to their previous type anymore
(eg vtkUnsignedCharArray cast will return nullptr now), but data has the same type as before
