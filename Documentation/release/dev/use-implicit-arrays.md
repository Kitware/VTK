## Use implicit arrays in more filters

Some filters now benefit from implementations using implicit arrays
in order to save array storage memory.

In particular:
- `vtkGenerateProcessIds`, `vtkBlockIdScalars` and `vtkOverlappingAMRLevelIdScalars` use a `vtkConstantArray`.
- `vtkCountFaces` and `vtkCountVertices` compute number of faces and vertices values on-demand instead of storing them.

As a consequence of this change, the output arrays of these filters cannot be cast to their previous type anymore
(eg vtkUnsignedCharArray cast will return nullptr now), but data has the same type as before
