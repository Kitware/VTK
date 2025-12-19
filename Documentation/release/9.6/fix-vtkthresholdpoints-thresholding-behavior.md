## Correct thresholding behavior in vtkThresholdPoints

`vtkThresholdPoints` now uses the same rules as `vtkThreshold` for both lower and upper bounds.
Before this change, points with values below the lower bound were kept when they should have been removed.
