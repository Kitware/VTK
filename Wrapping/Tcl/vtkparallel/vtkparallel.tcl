package require vtkio
package require vtkrendering

if {[::vtk::load_component vtkParallelTCL] == ""} {
    package provide vtkparallel 4.0
}

