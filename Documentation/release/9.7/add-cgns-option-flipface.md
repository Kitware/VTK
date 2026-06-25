# CGNS Polyhedron Flip Face Orientation

## Summary
It introduces a mechanism to correct face orientations that do not align with the expected CGNS cell conventions.
The **`FlipFaceNormals`** option of `vtkCGNSReader` can be used to resolve inconsistent face winding in polyhedral datasets
and let clipping and cutting filters work correctly.

## Changes
* **New Flag:** Added `FlipFaceNormals` to toggle the orientation of faces in `NFACE_n` elements.
* **Compatibility:** Provides a fix for "inside-out" cells caused by solvers that don't follow standard CGNS face-winding conventions.
* **Legacy :** To get the previous behavior of the reader set option should be set to `true`.

## API addition
* `SetFlipFaceNormals(bool)`
* `GetFlipFaceNormals()`
