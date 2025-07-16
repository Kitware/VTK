# HTG Surface Representation improvements

Cells outside the camera frustum are now decimated in 3D as well as 1D and 2D in HTG Surface Representation. The method to check their visibility has been
generified to work in all cases, including without parallel projection (which was required before), and with masks. Existing tests have been updated and new ones
created to cover these functionalities.

Some public members are now deprecated and have no effect:
  - `Set/GetCircleSelection`
  - `Set/GetBBSelection`
  - `Set/GetDynamicDecimateLevelMax`
  - `Set/GetScale`

Some protected members of `vtkAdaptiveDataSetSurfaceFilter` have been moved to private or completely removed:
- Removed:
  - `BBSelection`
  - `CircleSelection`
  - `DynamicDecimateLevelMax`
  - `LevelMax`
  - `LastCameraFocalPoint`
  - `LastCameraParallelScale`
  - `ParallelProjection`
  - `Radius`
  - `Scale`
  - `WindowBounds`
- Moved to private:
  - `Axis1`
  - `Axis2`
  - `Cells`
  - `Dimension`
  - `FixedLevelMax`
  - `InData`
  - `LastRendererSize`
  - `Mask`
  - `Orientation`
  - `OutData`
  - `Points`
  - `Renderer`
  - `ViewPointDepend`

The following methods have been moved to private:
  - `AddFace`
  - `ProcessLeaf1D`
  - `ProcessLeaf2D`
  - `ProcessLeaf3D`
  - `ProcessTrees`
  - `RecursivelyProcessTree1DAnd2D`
  - `RecursivelyProcessTree3D`
