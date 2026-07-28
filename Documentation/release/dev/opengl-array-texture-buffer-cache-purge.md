# Bound and purge the OpenGL array texture buffer cache

The per-context `vtkOpenGLArrayTextureBufferCache` used by the vertex-pulling
mappers (`vtkOpenGLLowMemoryPolyDataMapper`) previously grew without bound:
entries were only released when the OpenGL context was torn down, and each
entry kept its source `vtkDataArray` alive. Workflows that continually modify
geometry by re-executing filters, interactive mesh edits leaked one entry set
per update, which in WebAssembly exhausted the heap and crashed the
application.

The cache now purges automatically at the end of every
`vtkOpenGLRenderWindow::Render()`:

- Entries referenced by a live mapper ("pinned") are never evicted.
- Orphaned entries whose source array is owned solely by the cache are
  always dropped, together with their GPU resources.
- Remaining unpinned entries are evicted least-recently-used first until the
  cache fits `MaximumCacheSize` (default 256 MiB, `0` disables the budget).

New API on `vtkOpenGLArrayTextureBufferCache`:

- `SetMaximumCacheSize()` / `GetMaximumCacheSize()`: byte budget for unpinned
  entries.
- `GetCurrentCacheSize()`: GPU bytes currently held.
- `GetCPUMemorySize()`: host KiB pinned by cached source arrays.
- `RemoveUnusedTextureBuffers(window)`: run the purge explicitly.
- `RemoveTextureBuffer(array, window)`: drop all unpinned entries for one
  array.
