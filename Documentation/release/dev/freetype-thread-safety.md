## vtkFreeTypeTools: thread-safe rendering with per-thread FreeType instances

`vtkFreeTypeTools` is now thread-safe. Previously, the FreeType library, cache manager,
image cache, and cmap cache were shared singleton members, causing data races when
multiple threads rendered text concurrently.

The fix introduces `FTThreadLocalData`, a per-thread RAII struct that owns its own
FreeType library and cache instances via thread-local storage. The singleton itself is
now guarded by a mutex. A secondary fix addresses a double-free on glibc where
thread-local destructors ran before the global destructor of `vtkFreeTypeTools`, leading
to heap corruption; the destructor now checks whether the thread-local map has already
been torn down before accessing it.
