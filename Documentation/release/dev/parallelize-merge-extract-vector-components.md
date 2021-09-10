# Merge/ExtractVectorComponents are now multithreaded

`vtkMergeVectorComponents` and `vtkExtractVectorComponents` filters have been multithreaded using `vtkSMPTools`.

Additionally, the `TestMergeVectorComponents` test has been augmented to check if different types of arrays can be used.
