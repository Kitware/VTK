package require vtkio
package require vtkrendering

if {[::vtk::load_component vtkHybridTCL] == ""} {
    package provide vtkhybrid 4.0
}
