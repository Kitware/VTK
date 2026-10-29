## Configuring VTK with Viskores on Kokkos/HIP works again

Configuring VTK with the vendored Viskores on a Kokkos built for HIP failed with
`Kokkos could not find original compiler: ''`. Modules with Viskores device
code (`VTK::AcceleratorsVTKmCore` among them) compile it through
`kokkos_compilation`, which needs the compiler Kokkos was built with, and
`find_package(Kokkos)` defines that only in Viskores' own directory. VTK now
keeps it where Viskores is configured and passes it on.
