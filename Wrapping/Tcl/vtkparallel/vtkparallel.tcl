package require vtkio
package require vtkrendering

if {[info commands vtkThreadedController] != "" ||
    [::vtk::load_component vtkParallelTCL] == ""} {
    package provide vtkparallel 4.0
}

