## Make vtkUniforms a non abstract class.

The `vtkUniforms` class is now not an abstract class. All the pure virtual functions are implemented as no-op
and print a warning if invoked while using a rendering factory that does not override `vtkUniforms`.

This is required for compatibility with the new webgpu backend where it is not possible to set global uniforms
outside a bind group.
