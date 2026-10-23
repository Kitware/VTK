## WebGPU builds no longer require Dawn

Configuring with `VTK_ENABLE_WEBGPU` no longer requires Dawn to be installed.
The backend speaks the standard WebGPU C API and resolves its entry points at
runtime, so Dawn is needed only to declare Dawn's own extensions.

Ask for those extensions with the new `VTK_USE_DAWN_WEBGPU` option, which
defaults to `OFF`. Turning it on makes Dawn a hard requirement: configuration
fails with a clear error when it cannot be found, rather than quietly building
without the extensions.

The build used to look for Dawn unconditionally and declare the extensions
whenever it happened to find one, so the same source tree could produce
different builds on two machines. A build that relied on that detection now
needs `-DVTK_USE_DAWN_WEBGPU=ON`.
