## Modified API for setting threshold bounds and method

From VTK 9.1, the following methods from `vtkThreshold` are deprecated:

- `vtkThreshold::ThresholdByLower`
- `vtkThreshold::ThresholdByUpper`
- `vtkThreshold::ThresholdBetween`

Instead, setters for the threshold bounds and for the threshold method have been added:

- `vtkThreshold::SetLowerThreshold`
- `vtkThreshold::SetUpperThreshold`
- `vtkThreshold::SetThresholdFunction`
- `vtkThreshold::GetThresholdFunction`

A corresponding `enum ThresholdType` has been added for the threshold methods.

As such, the threshold filter attributes should be defined as follows:

- Before VTK 9.1

  ```
  vtkThreshold::ThresholdByLower(lower);
  ```

  ```
  vtkThreshold::ThresholdByUpper(upper);
  ```

  ```
  vtkThreshold::ThresholdBetween(lower, upper);
  ```

- From VTK 9.1

  ```
  vtkThreshold::SetThresholdFunction(vtkThreshold::THRESHOLD_LOWER);
  vtkThreshold::SetLowerThreshold(lower);
  ```

  ```
  vtkThreshold::SetThresholdFunction(vtkThreshold::THRESHOLD_UPPER);
  vtkThreshold::SetUpperThreshold(upper);
  ```

  ```
  vtkThreshold::SetThresholdFunction(vtkThreshold::THRESHOLD_BETWEEN);
  vtkThreshold::SetLowerThreshold(lower);
  vtkThreshold::SetUpperThreshold(upper);
  ```
