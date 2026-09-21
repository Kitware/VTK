## DLPack interop for VTK arrays

VTK arrays can now be exchanged with DLPack-speaking libraries -- CuPy,
PyTorch, JAX, NumPy, TensorFlow -- without copying, on the host or on a GPU.

```python
from vtkmodules.util.dlpack_support import vtk_to_dlpack, dlpack_to_vtk

import cupy
gpu = cupy.from_dlpack(vtk_to_dlpack(vtk_array))   # VTK -> CuPy
back = dlpack_to_vtk(gpu)                          # CuPy -> VTK
```

Both directions are views, so both sides look at the same memory and a write
through one is visible through the other. Lifetime is handled by the protocol
rather than by the caller: an exported array stays alive for exactly as long
as the consumer's view of it, and an imported array holds the DLPack capsule
open so the producer's buffer cannot be collected while VTK is using it.

The module also provides `DLPackArray`, a wrapper exposing `__dlpack__` and
`__dlpack_device__` for libraries that consume the protocol implicitly (for
example `numpy.from_dlpack`), and `supported_device_types()`.

### Describing array memory

Two pieces of C++ API make this possible, and are useful on their own.

`vtkMemoryDescriptor` describes one buffer: a pointer, a size in bytes, a
memory space (`"host"`, `"cuda"`, `"hip"`, ...) and a role. It can hold a
reference to the VTK object that owns the memory, or a release callback for
memory VTK does not own, so a consumer can keep a buffer alive without
knowing anything about VTK's object model.

`vtkDataArray::NewMemoryDescriptors()` reports the buffers backing an array.
The caller takes ownership of the returned collection -- hence `New` rather
than `Get`.

```cpp
vtkNew<vtkFloatArray> array;
// ...
vtkSmartPointer<vtkCollection> descriptors;
descriptors.TakeReference(array->NewMemoryDescriptors());
```

Array-of-structs reports one `"data"` buffer, struct-of-arrays reports one
`"component_0"`, `"component_1"`, ... buffer per component, and a
`vtkmDataArray` reports its device memory without pulling it back to the
host. Only real storage is described, so a descriptor is always a view:
arrays with no storage -- implicit and computed arrays -- report nothing at
all rather than a flattened temporary, and callers can tell the difference.

### Wrapping external memory

`vtkmDataArrayFactory` builds a `vtkmDataArray` over host or device memory
VTK did not allocate, so data produced elsewhere can be fed into
Viskores-accelerated filters and stay where it is. One buffer gives AoS,
`NumberOfComponents` buffers give SoA. The factory takes each descriptor's
release and installs it as the deleter on the Viskores buffer, so the
producer's hold is dropped exactly when Viskores is finished with the memory.
External buffers are installed as non-resizable; a filter that tries to grow
one gets `viskores::cont::ErrorBadAllocation` rather than a write past the
end of the caller's allocation.

### Requirements

Host arrays work with plain VTK. Device arrays require VTK built with
`VTK_ENABLE_VISKORES` so that `vtkmDataArrayFactory` is available to wrap
device memory; importing a device tensor without it raises `RuntimeError`
rather than silently copying to the host.

Metal tensors are imported as host arrays: a shared-storage `MTLBuffer` on
Apple Silicon is unified memory, so its pointer is host-readable and the wrap
is still zero-copy. Private-storage buffers have no host-readable pointer and
are rejected. VTK never exports Metal memory, because no VTK array holds it.
