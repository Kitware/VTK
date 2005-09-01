package require -exact vtkrendering 5.1

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkVolumeRenderingTCL 5.1]} {
    package provide vtkvolumerendering 5.1
  }
} else {
  if {[info commands vtkEncodedGradientShader] != "" ||
    [::vtk::load_component vtkVolumeRenderingTCL] == ""} {
    package provide vtkvolumerendering 5.1
  }
}
