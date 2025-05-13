## Append filters: Multithread and Improve Performance

The `vtkAppendFilter` and `vtkAppendPolyData` filters have been re-writen and multithreaded, and their performance
has been significantly improved. `vtkMergeBlocks` and `vtkAppendDataSets` which use `vtkAppendFilter` and
`vtkAppendPolyData` internally have also been improved.
