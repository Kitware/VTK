## Make vtkOpenXRManager private

vtkOpenXRManager is now a private class and should not be used outside of VTK.
Breaking changes are:
- vtkOpenXRManager has been moved to vtk::detail::vtkOpenXRManager.
- vtkOpenXRManager::InstanceVersion has been replaced by vtkOpenXRRenderWindow::InstanceVersion
- vtkOpenXRManager::QueryInstanceVersion has been replaced by vtkOpenXRRenderWindow::QueryInstanceVersion
- vtkOpenXRManagerConnection::ConnectToRemote and vtkOpenXRManagerGraphics::CheckGraphicsRequirements scope has been moved from public to protected
- vtkOpenXRRenderWindowInteractor::GetHandPose and vtkOpenXRRenderWindowInteractor::ConvertOpenXRPoseToWorldCoordinates have been removed.
