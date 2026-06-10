## vtkRandomHyperTreeGridSource: consistent behavior in distributed

Rework `vtkRandomHyperTreeGridSource` so that the output of the source is consistent in serial and in parallel, with or without masks. Performance of the filter was also improved.

Generated HTG *without* masking in *serial* is unchanged. Distributed HTG is now the same as the serial version. Mask generation was rewritten from the ground up and simplified to get consistent masking in serial and distributed, which was not possible with the current algorithm. Now, only non-refined cells can be masked.

Public API is otherwise unchanged.
