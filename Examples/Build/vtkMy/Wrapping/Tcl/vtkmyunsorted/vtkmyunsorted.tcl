package require vtkmyimaging
package require vtk

#
# Here you should pick the name of one your imaging local classes
# instead of vtkBar2.
#

if {[info commands vtkBar2] != "" ||
    [::vtk::load_component vtkmyUnsortedTCL] == ""} {
    package provide vtkmyunsorted 4.0
}
