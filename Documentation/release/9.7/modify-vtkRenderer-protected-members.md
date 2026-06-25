## Modify `vtkRenderer`'s protected member

`vtkRenderer::PropArrayCount` protected member was removed from the renderer. Moreover, `vtkRenderer::PropArray` has been replaced by an `std::vector<vtkProp*>` in order to preserve the `PropArrayCount` value and its elements.

The behavior of this array has changed since the implementation of the skybox blur. When populating `PropArray`, if one of the actor is a `vtkSkybox`, it is now kept by `vtkRenderer::BackgroundProp` and not pushed in the `PropArray` vector.
