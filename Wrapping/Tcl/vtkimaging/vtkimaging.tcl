package require vtkfiltering

if {[::vtk::load_component vtkImagingTCL] == ""} {
    package provide vtkimaging 4.0
}
