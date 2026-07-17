# Add the new render pass StencilMaskPass

It is now possible to use a geometry to occlude specific regions of a scene using the new `StencilMaskPass`.

To choose which actor(s) will occlude the scene, assign the information key `vtkStencilMaskPass::GLStencilWrite()` to them.

Note that this pass also writes to the Depth Buffer (Z-buffer) during the mask creation. If you need to reset the depth state afterward, you can clear it by adding a vtkClearZPass right after the StencilMaskPass in your render pipeline.

Warning: For this pass to work correctly, the stencil buffer must be enabled on the `vtkRenderWindow` by calling the `StencilCapableOn()` method.
