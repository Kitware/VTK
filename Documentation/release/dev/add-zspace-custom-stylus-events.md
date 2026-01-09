## Add zSpace custom stylus events

zSpace module now have the ability to throw custom stylus events. These events can be controlled by using the 2 functions in `vtkZSpaceSDKManager`:
- `vtkZSpaceSDKManager::SetInvokeDefaultStylusEvents(bool)`: if true, the events needed for the default behavior will be emitted.
- `vtkZSpaceSDKManager::SetInvokeCustomStylusEvents(bool)`: if true, user events will be emitted containing the data of the state that changed.

Due to the new behavior introduced, `vtkZSpaceRenderWindowInteractor` has been refactored, making the event dispatch behavior more generic. Thus, the following public functions were removed:
- `vtkZSpaceRenderWindowInteractor::OnLeftButtonDown`, `vtkZSpaceRenderWindowInteractor::OnLeftButtonUp`
- `vtkZSpaceRenderWindowInteractor::OnMiddleButtonDown`, `vtkZSpaceRenderWindowInteractor::OnMiddleButtonUp`
- `vtkZSpaceRenderWindowInteractor::OnRightButtonDown`, `vtkZSpaceRenderWindowInteractor::OnRightButtonUp`
