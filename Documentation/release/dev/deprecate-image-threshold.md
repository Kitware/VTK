## Deprecate `vtkImageThreshold` and switch to `vtkImageBinaryThreshold`

`vtkImageThreshold` is now deprecated and replaced by `vtkImageBinaryThreshold`. The new class changes the way the API works for more consistency:

- Use `SetThresholdFunction` to switch to the 3 different thresholding modes: `vtkImageBinaryThreshold::THRESHOLD_LOWER`, `vtkImageBinaryThreshold::THRESHOLD_UPPER`, `vtkImageBinaryThreshold::THRESHOLD_BETWEEN`.
- `ReplaceIn` and `ReplaceOut` now have to be set manually to actually replace the output values.
