## vtkHDFWriter: improve chunk size management

When writing VTKHDF files using `vtkHDFWriter`, chunk size is now constrained between a minimum pre-configured (100) value, and the minimum between configured chunk size and the dataset size. This means that a small dataset will not waste too much space anymore because of a higher chunk size. However, this configuration can be sub-optimal when the dataset changes size a lot over time, or when parallel distribution is unequal.
