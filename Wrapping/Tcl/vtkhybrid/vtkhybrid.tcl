package require vtkio
package require vtkrendering

if {[info commands vtkEarthSource] != "" ||
    [::vtk::load_component vtkHybridTCL] == ""} {
    package provide vtkhybrid 4.0
}
