# Fix ANARI device handle lifetime

`anari::Device` handle lifetime is now internally handled by the `vtkAnariDevice` class. All classes using the ANARI device now uses a `vtkSmartPointer` to prevent `anari::Device` access to fail and cause crash.

These modifications are breaking the ANARI module API:
- `vtkAnariSceneGraph::GetDeviceHandle` has been replaced by `vtkAnariSceneGraph::GetDevice` and now returns a `vtkSmartPointer<vtkAnariDevice>` instead.
- `vtkAnariDevice::GetAnaridDeviceExtensions` has been renamed `vtkAnariDevice::GetExtensions` for more readability.
- `vtkAnariDevice::GetAnariDeviceExtensionString` has been renamed `vtkAnariDevice::GetExtensionStrings` for more readability.
