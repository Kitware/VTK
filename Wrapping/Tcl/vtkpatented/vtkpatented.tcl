package require vtkio
package require vtkrendering

if {[info commands vtkKitwareContourFilter] != "" ||
    [::vtk::load_component vtkPatentedTCL] == ""} {
    package provide vtkpatented 4.0
}

