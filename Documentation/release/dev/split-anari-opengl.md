# Split ANARI core from its OpenGL implementation

ANARI is now split into 2 different modules. The goal of these modifications is to be able to use ANARI without a dependency to OpenGL. ANARI implementation in VTK is located in 2 distinct modules:
- `VTK::RenderingAnariCore`: the ANARI only implementation that does not depend on OpenGL. This module can perform rendering using an ANARI backend. The rendering happens offscreen, using `vtkAnariRenderWindow`.
- `VTK::RenderingAnariOpenGL`: the implementation of the OpenGL layer that can be used to render a scene with the ANARI backend and display the result onscreen. This module essentially implements the `vtkAnariPass` that can be attached to the renderer to copy the ANARI frame result to the OpenGL render window.

These changes are not backward-compatible with the previous implementation.
