## Add find_package hints for OpenVR

The `vtkInstallCMakePackageHelpers` CMake module has been updated
to include hints for finding `OpenVR` libraries.

This allows to successfully build client projects against a VTK build
tree where the `RenderingOpenVR` module has been enabled.
