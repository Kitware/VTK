## Introduce StableClip to `vtkQuadraticTetra`

A new `vtkNonLinearCell::StableClip` method and an implementation for `vtkQuadraticTetra::StableClip` is now available. The goal of this special clip is to only decompose the cell if it is actually clipped. Otherwise, keep the non-linear cell in its entirety. This method is now used in vtkClipDataSet for non-linear cells when the `NonLinearStableClip` boolean property is set to `true` (which is the case by default). This behavior produces a change in the way that non-linear cells have been treated up until now by default.

Given that the only cell that implements this for now is `vtkQuadraticTetra`, other non-linear cells will keep their usual behavior (i.e. be decomposed everywhere) until their `StableClip` methods are implemented.

> WARNING: this approach to clipping non-linear cells will lead to topological holes between decomposed cells and the remaining non-linear cells
