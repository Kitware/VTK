# Improve OpenGL shadow pass

`vtkShadowMapPass::ShadowPass()` information key is unused and has been deprecated.
Three new information have been added:
- `vtkShadowMapBakerPass::OCCLUDER()`: can be added to `vtkProp` instances in order to specify if it should be rendered during the shadow maps bake pass.
- `vtkShadowMapBakerPass::CAST_SHADOWS()`: can be added to `vtkLight` instances in order to specify if it should generate a shadow map.
- `vtkShadowMapPass::RECEIVER()`: can be added to `vtkProp` instances in order to specify if it should take shadow maps into account when rendered.

Fixed an issue in `calcShadow` to return a correct value if the requested texel has not been written during the shadow map bake pass.
Force the shadow map camera to use a 1:1 ratio to avoid shadow map distortions.
`SetOpaqueSequence()` and `GetOpaqueSequence()` has been deprecated and renamed to `SetDelegatePass()` and `GetDelegatePass()` on both `vtkShadowMapBakerPass` and `vtkShadowMapPass`.
`OpaqueSequence` protected member have been removed.
`ShadowMaps`, `LightCameras`, `CompositeZPass` and `FrameBufferObject` in `vtkShadowMapBakerPass` protected members are now private.
`vtkShadowMapBakerPass::GetNeedUpdate()` and `vtkShadowMapBakerPass::SetUpToDate()` functions are now private.
`vtkShadowMapBakerPass::GetShadowMaps()` and `vtkShadowMapBakerPass::GetLightCameras()` return a reference instead of a pointer now and are documented as public API.
