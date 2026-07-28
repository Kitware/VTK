Render passes that are not specific to OpenGL have been moved from `RenderingOpenGL2` to `RenderingCore`, so that they will be able to be used with WebGPU. The moved classes are as follows:

- `vtkDefaultPass`
- `vtkLightsPass`
- `vtkOpaquePass`
- `vtkOverlayPass`
- `vtkRenderPassCollection`
- `vtkSequencePass`
- `vtkTranslucentPass`
- `vtkVolumetricPass`

Python imports of the above classes must be changed to import from `vtkmodules.vtkRenderingCore` instead of `vtkmodules.vtkRenderingOpenGL2`.
