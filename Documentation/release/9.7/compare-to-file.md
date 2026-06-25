## New vtkTestUtilities API to test data against file

`vtkTestUtilities::CompareWithFile()` is a new static method
to easily compare any `vtkDataObject` in memory against a VTKHDF file.

`vtkTestUtilities::RegressionTest()` is a static method that wraps
`CompareWithFile()`. Similar to image baseline comparison framework,
it writes the tested `vtkDataObject` under the testing temporary directory
on failure (either file access failure or comparison failure).
Thus it is easy to create the data baseline file without changing any code
and it is easy to debug by inspecting the temporary file.

When porting a test to this new API, do not forget to remove previous
(data or image) baseline from the repository!
