## GPU arrays without Viskores: vtkDeviceDataArray

VTK can now hold memory another library allocated on a GPU without being built
with Viskores or any GPU toolkit. `vtkDeviceDataArray` holds interleaved values
described by a `vtkMemoryDescriptor`, and `vtkDeviceArrayFactory` makes one;
`vtkmodules.util.dlpack_support` uses them for CUDA tensors, so
`dlpack_to_vtk` of a CUDA tensor works in any VTK on a machine with NVIDIA's
driver.

The array gives the memory back as it is -- `GetMemorySpace()`,
`GetDeviceVoidPointer()`, `NewMemoryDescriptors()`, and so `vtk_to_dlpack` --
so GPU filters in a pipeline exchange it without copies. VTK code that reads
values on the host reads a copy of the whole buffer, made on first use and
again after `Modified()`. The array is read-only on the host and cannot grow:
both are refused with an error, and `NewInstance()` returns an ordinary host
array of the same type.

VTK does not link against CUDA for this: the driver (`libcuda.so.1`,
`nvcuda.dll`) is loaded the first time it is needed. HIP and Level Zero memory
still need Viskores. `VTK_DLPACK_DEVICE_ARRAYS=viskores` makes
`dlpack_support` prefer Viskores' arrays where both are available.
