## `vtkOpenXRManager::Initialize` and `vtkOpenXRManager::Finalize` has been marked as internal APIs

These functions were designed as an internal API of `vtkOpenXRRenderWindow`.
Signature of `vtkOpenXRManager::Initialize` has changed to better express this.

## `vtkOpenXRManager::(Set|Get)UseDepthExtension` has been moved to `vtkOpenXRRenderWindow`

`vtkOpenXRManager::(Set|Get)UseDepthExtension` has been moved `vtkOpenXRRenderWindow`.
`vtkOpenXRManager` functions are still present, but no-op and deprecated.
