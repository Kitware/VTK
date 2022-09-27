## Fix Surface LIC crash when rendering lines as tubes or points as spheres

Missing `normalMatrix` uniform definition in the fragment shader makes ParaView crash during the shader compilation.
The `vtkOpenGLPolyDataMapper` does not substitute the uniform if the `Rendering lines as tubes` or `Rendering points as spheres` option is checked.
Fixed by asking the `vtkSurfaceLICMapper` to do it before calling the superclass (`vtkOpenGLPolyDataMapper`).
