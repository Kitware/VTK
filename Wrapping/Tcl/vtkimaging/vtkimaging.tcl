package require vtkfiltering

if {[info commands vtkImageFFT] != "" ||
    [::vtk::load_component vtkImagingTCL] == ""} {
    package provide vtkimaging 4.0
}
