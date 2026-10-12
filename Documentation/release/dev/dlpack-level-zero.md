## DLPack and Viskores interop on Level Zero

`vtkmodules.util.dlpack_support` now imports and exports `kDLOneAPI` tensors
when VTK is built with Viskores on Kokkos' SYCL backend running on a Level
Zero device.

A Level Zero USM pointer is only meaningful inside the context it was
allocated from, and DLPack has no field for a context. Instead of carrying
one with every tensor, VTK reports the context its device memory lives in
once, and a producer allocates there:

```python
from vtkmodules.util.dlpack_support import level_zero_handles

handles = level_zero_handles()   # {"driver": ..., "device": ..., "context": ...}
```

The handles are those of the SYCL queue Viskores runs on, which Kokkos
creates in the SYCL platform's default context. They belong to the SYCL
runtime and must not be destroyed. `vtkmDataArrayFactory` exposes the same
values as `GetLevelZeroDriverHandle()`, `GetLevelZeroDeviceHandle()` and
`GetLevelZeroContextHandle()`.

Supporting this needed several smaller changes:

- `vtkDataArray::MemorySpace` gains `LevelZeroDeviceMemory`.
- `vtkMemoryDescriptor` addresses are now `vtkTypeUInt64`. Device allocators
  return addresses in the upper half of the range, and the wrapped `Set()`
  raised `OverflowError` on each of them while the parameter was signed.
- `vtkmDataArrayFactory` accepts a `"level_zero"` buffer only when the SYCL
  runtime recognises the pointer as USM device or shared memory. A Kokkos
  build now accepts only the memory space it was built for, where it
  previously accepted both `"cuda"` and `"hip"` whichever backend it had.
- `vtkmDataArray::NewMemoryDescriptors()` reports the memory an array
  already occupies. On a build with a device it used to request the device
  pointer unconditionally, which copied host-only data to the device and
  described that copy as the array's memory.

The driver used for development does not distinguish contexts when asked
about a pointer, so the import check separates device memory from host
memory but cannot prove that a pointer came from the reported context.
