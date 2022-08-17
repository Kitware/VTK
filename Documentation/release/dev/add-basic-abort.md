# Add basic abort functionality to VTK

VTK's interruption method has been updated to use
`CheckAbort`. `CheckAbort` will check the current filter's
`AbortExecute` flag as well as any upstream filter's `AbortExecute`
flag. If any are set, the filter will output empty data and tell
downstream filters to abort as well. Currently, `vtkContourGrid`,
`vtkClipDataSet`, `vtkShrinkFilter`, and `vtkRTAnalyticSource`.
