## WebGPU Backend Support for Context2D

The WebGPU rendering backend support VTK's 2D rendering API with the implementation of the `vtkWebGPUContextDevice2D` class.

This allows VTK's 2D elements—such as `vtkChart`, UI overlays, math text, and standard 2D primitives—to be rendered using the WebGPU graphics API.
Supported features include point sprites, dynamic thick and thin line drawing, polygons, clipping, and texture handling.
