# vtkToneMappingPass for WebGPU

`vtkToneMappingPass` moved from `RenderingOpenGL2` to `RenderingCore`, and the WebGPU backend now
implements it as `vtkWebGPUToneMappingPass`, with the same operators and settings as the OpenGL
implementation, now `vtkOpenGLToneMappingPass`. `vtkToneMappingPass::New()` returns the
implementation of the active backend through the object factory. With WebGPU, the pass tone maps
everything its renderer draws, rather than only what its delegate draws.

Python imports of `vtkToneMappingPass` must be changed to import from
`vtkmodules.vtkRenderingCore` instead of `vtkmodules.vtkRenderingOpenGL2`. The class now derives
from `vtkRenderPass` rather than `vtkImageProcessingPass`, and keeps its `DelegatePass` and tone
mapping settings.
