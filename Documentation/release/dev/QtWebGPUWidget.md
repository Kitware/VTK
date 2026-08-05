# QVTKWebGPUWidget Native Surface Integration

`QVTKWebGPUWidget` now renders WebGPU content directly into the Qt widget
surface instead of creating a separate native window. The integration works by
letting `vtkWebGPURenderWindow` accept a custom platform surface descriptor,
which allows the widget's native window handle to be passed through to WebGPU
during initialization.

`vtkWebGPURenderWindow` now exposes:

- `void SetCustomSurfaceDescriptor(const wgpu::SurfaceDescriptor* descriptor)`
- `const wgpu::SurfaceDescriptor* GetCustomSurfaceDescriptor() const`

When a custom surface descriptor is provided before `Initialize()` is called,
the render window uses it directly and bypasses the usual platform-specific
path that creates a new native window. In practice, `QVTKWebGPUWidget` builds
the appropriate surface descriptor from the Qt native window handle, attaches
it to the render window, and then initializes rendering so WebGPU content
appears inside the widget itself.

Because the custom descriptor is tied to the active platform and WebGPU
backend, it is not included in VTK's marshalling or serialization system. It
cannot be safely serialized, replayed, or restored across sessions, so
applications that persist render windows should not expect this state to
survive serialization.
