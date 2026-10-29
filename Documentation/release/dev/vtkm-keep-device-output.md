## Viskores filter output stays on the GPU

Arrays a Viskores-accelerated filter computes on a GPU now come back as
`vtkmDataArray`s over the device memory, on every GPU backend. A pipeline of
Viskores filters therefore hands its data from one filter to the next on the
device, and only moves it to the host when something reads it there.

Previously this only happened on Kokkos' CUDA and HIP backends. Everywhere
else the output was assumed to be in memory the host shares, and was turned
into an ordinary VTK array by taking host ownership of it, so each filter
moved its output to the host and the next moved it straight back:

- On Kokkos' SYCL backend (Level Zero), every output array was copied from
  device to host.
- On Viskores' native CUDA backend, with managed memory, every output array
  was prefetched to the host.

Whether to keep an array on the device is now decided by where its data are,
not by which backend was compiled in: data a GPU device wrote and the host
has not seen stay where they are, and data in host memory -- including
anything computed by the serial, TBB or OpenMP devices -- still becomes an
ordinary VTK array without a copy.

On a native CUDA build this changes the type of filter output from
`vtkAOSDataArrayTemplate` (and friends) to `vtkmDataArray`. Code that only
uses the `vtkDataArray` API is unaffected; code that down-casts the output to
a concrete VTK array type should check for `vtkmDataArray` as well.
