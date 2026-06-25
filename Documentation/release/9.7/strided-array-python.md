## VTKStridedArray numpy-compatible Python mixin

`vtkStridedArray` now has a numpy-compatible Python mixin (`VTKStridedArray`)
that provides arithmetic operators, reductions, ufunc support, indexing,
and `to_numpy()`. Strided arrays coming from VTK (e.g. via Catalyst/Conduit
zero-copy paths) are automatically wrapped with this mixin.

When the underlying buffer is accessible, `numpy.asarray()` returns a
**zero-copy strided numpy view** — no data is copied. The VTK stride and
offset map directly to numpy strides, so all numpy operations work natively
on the buffer memory.

New `ConstructBackend(vtkAbstractBuffer*, ...)` overloads allow constructing
strided arrays from Python using a `vtkBuffer` as the buffer source.
The buffer is stored with reference counting to keep the memory alive,
since the raw-pointer overloads are not Python-wrappable and do not
manage buffer lifetime. `GetBufferSource()` provides access to the stored
buffer object.
