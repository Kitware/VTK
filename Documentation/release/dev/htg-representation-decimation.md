## HTG Surface Representation improvements

- Rewrite french comments to english
- Generify cell visibility test to work in 3D as well as 1D and 2D, and not necessarily with parallel projection.
- Support masks in 2D and 3D
- Update and add vtkAdaptiveDataSetSurfaceFilter tests

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
