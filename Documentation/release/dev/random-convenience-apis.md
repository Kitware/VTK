## Convenience APIs for random numbers

The following APIs have been added:

  - `vtkGaussianRandomSequence::GetNextScaledValue`
  - `vtkMinimalStandardRandomSequence::GetNextRangeValue`
  - `vtkRandomSequence::GetNextValue`

These methods advance the sequence and return the new random number. This
allows code which is using random numbers to not have to manually call
`vtkRandomSequence::Next` for each value.
