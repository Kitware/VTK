## vtkHDFWriter: improve chunk size management

When writing VTKHDF files using `vtkHDFWriter`, chunk size is now constrained between a minimum pre-configured (100) value, and the minimum between configured chunk size and the dataset size. This means that a small dataset will not waste too much space anymore because of a higher chunk size. However, this configuration can be sub-optimal when the dataset changes size a lot over time, or when parallel distribution is unequal.

## Improved distributed composite HyperTreeGrid support

Writing and reading HyperTreeGrids as part of a multi-block compositer dataset has been improved.
You can now write and read HyperTreeGrid with a null partition on rank 0 without issues.

## ImageData: 2D and extent support

The ImageData writer & reader now supports 2D and 1D Images, and has improved support for reading sub-extents.

## ImageData temporal reading cache fix

ImageData temporal cache had a cache invalidation issue when switching between time steps. The cache is now properly cleared when time step change.

## vtkHDFReader: cache fixes

Fixed `vtkHDFReader` caching when re-enabling point or cell arrays, changing structured extents, and reading structured composite leaves with matching array names and extents.

## vtkHDFWriter: distributed single-file mode

You can now write UnstructuredGrid and PolyData partitions collectively into one HDF5 file,
including polyhedra, temporal data, and composites containing these leaf types.

Previously, parallel writing was only available using one file per rank,
referenced by the 'main' file using HDF5 external links.
Using a single file and collective writes is a more performant approach at scale.

`UseExternalDistributed` is now set to false by default.

## vtkHDFWriter: append timesteps

The new `AppendTimeSteps` option (off by default) can append new time steps to an existing file,
given that this file already contains temporal metadata, and that the data type and field arrays are fully compatible.
Only UnstructuredGrid and PolyData and composite types are supported.
`SetUseExternal*` options are not used

Set `CurrentTimeValue` before each call to `Write()` to specify an explicit time value.
