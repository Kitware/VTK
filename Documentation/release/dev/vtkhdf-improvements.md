## vtkHDFWriter: improve chunk size management

When writing VTKHDF files using `vtkHDFWriter`, chunk size is now constrained between a minimum pre-configured (100) value, and the minimum between configured chunk size and the dataset size. This means that a small dataset will not waste too much space anymore because of a higher chunk size. However, this configuration can be sub-optimal when the dataset changes size a lot over time, or when parallel distribution is unequal.

## Improved distributed composite HyperTreeGrid support

Writing and reading HyperTreeGrids as part of a multi-block compositer dataset has been improved.
You can now write and read HyperTreeGrid with a null partition on rank 0 without issues.

## ImageData: 2D and extent support

The ImageData writer & reader now supports 2D and 1D Images, and has improved support for reading sub-extents.
