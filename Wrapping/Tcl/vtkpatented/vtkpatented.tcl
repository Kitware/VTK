package require vtkio
package require vtkrendering

if {[::vtk::load_component vtkPatentedTCL] == ""} {
    package provide vtkpatented 4.0
}

