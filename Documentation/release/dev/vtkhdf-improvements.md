## ImageData temporal reading cache fix

ImageData temporal cache had a cache invalidation issue when switching between time steps. The cache is now properly cleared when time step change.

## ImageData: 2D and extent support

The ImageData writer & reader now supports 2D and 1D Images, and has improved support for reading sub-extents.

## vtkHDFReader: cache fixes

Fixed `vtkHDFReader` caching when re-enabling point or cell arrays, changing structured extents, and reading structured composite leaves with matching array names and extents.