# vtkToneMappingPass moved to RenderingCore

`vtkToneMappingPass` moved from `RenderingOpenGL2` to `RenderingCore`, so that rendering backends
other than OpenGL can implement it. The OpenGL implementation is `vtkOpenGLToneMappingPass`, which
`vtkToneMappingPass::New()` returns through the object factory. Python imports of
`vtkToneMappingPass` must be changed to import from `vtkmodules.vtkRenderingCore` instead of
`vtkmodules.vtkRenderingOpenGL2`. The class now derives from `vtkRenderPass` rather than
`vtkImageProcessingPass`, and keeps its `DelegatePass` and tone mapping settings.
