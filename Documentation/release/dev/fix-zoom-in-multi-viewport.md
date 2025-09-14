## Fix MouseWheel events in WASM for multi-viewport setups

Fixed an issue where mouse wheel event coordinates were incorrectly scaled in `vtkWebAssemblyRenderWindowInteractor`. When the `window.devicePixelRatio` value was not equal to 1 in multi-viewport setups, it caused a different renderer to zoom its contents instead of the renderer which was under the mouse cursor. The `ProcessEvent` method now properly handles coordinate scaling for MouseWheel events, ensuring consistent zoom behavior across all viewports.
