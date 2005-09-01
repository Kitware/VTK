package require -exact vtkrendering 5.0

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkVolumeRenderingTCL 5.0]} {
    package provide vtkvolumerendering 5.0
  }
} else {
  if {[info commands vtkEncodedGradientShader] != "" ||
    [::vtk::load_component vtkVolumeRenderingTCL] == ""} {
    package provide vtkvolumerendering 5.0
  }
}
