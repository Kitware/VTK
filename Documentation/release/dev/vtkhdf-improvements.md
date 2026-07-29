## ImageData temporal reading cache fix

ImageData temporal cache had a cache invalidation issue when switching between time steps. The cache is now properly cleared when time step change.

## ImageData: 2D and extent support

The ImageData writer & reader now supports 2D and 1D Images, and has improved support for reading sub-extents.
