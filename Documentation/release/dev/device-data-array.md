## GPU arrays without Viskores: vtkDeviceDataArray

VTK can now hold memory another library allocated on a GPU without being built
with Viskores or any GPU toolkit. `vtkDeviceDataArray` holds interleaved values
described by a `vtkMemoryDescriptor`, and `vtkDeviceArrayFactory` makes one;
`vtkmodules.util.dlpack_support` uses them for CUDA, HIP and Level Zero
(oneAPI) tensors, so `dlpack_to_vtk` of a GPU tensor works in any VTK on a
machine with the vendor's driver.

The array gives the memory back as it is -- `GetMemorySpace()`,
`GetDeviceVoidPointer()`, `NewMemoryDescriptors()`, and so `vtk_to_dlpack` --
so GPU filters in a pipeline exchange it without copies. An exported tensor's
DLPack `device_id` is now the device its memory is on
(`vtkDeviceArrayFactory::GetDeviceOrdinal()`, for Viskores' arrays too), not
0, unless the caller passes one. VTK code that reads
values on the host reads a copy of the whole buffer, made on first use and
again after `Modified()`. The array is read-only on the host and cannot grow:
both are refused with an error, and `NewInstance()` returns an ordinary host
array of the same type.

VTK links against no GPU toolkit for this. The vendor's library is loaded the
first time it is needed, and `vtkDeviceArrayFactory::IsMemorySpaceAvailable()`
says whether it can be:

- CUDA: the driver, `libcuda.so.1` (`nvcuda.dll`).
- HIP: ROCm's runtime, `libamdhip64.so.7` or `.so.6` (`amdhip64_7.dll`,
  `amdhip64_6.dll`); the copy the process has loaded already, if any.
- Level Zero: the loader, `libze_loader.so.1` (`ze_loader.dll`).

A Level Zero pointer means something only in the context that allocated it,
and DLPack carries no context, so VTK keeps one Level Zero context per
process: one given to `vtkDeviceArrayFactory::SetLevelZeroContext()` (a SYCL
runtime's default context, say), else Viskores' where VTK has it, else one VTK
makes on the first GPU the loader reports. `GetLevelZeroDriverHandle()`,
`GetLevelZeroDeviceHandle()` and `GetLevelZeroContextHandle()` report it, and
`dlpack_support.level_zero_handles()` returns them: a producer allocates in
that context.

In VTK built with Viskores, `dlpack_support` still gives device memory of a
kind Viskores was built for to Viskores' arrays, so Viskores' filters run on
it, and uses `vtkDeviceDataArray` for the rest. `VTK_DLPACK_DEVICE_ARRAYS=vtk`
prefers `vtkDeviceDataArray` where both could hold the memory, and
`vtkmDataArrayFactory::IsMemorySpaceAvailable()` says which memory this
build's Viskores can hold.
