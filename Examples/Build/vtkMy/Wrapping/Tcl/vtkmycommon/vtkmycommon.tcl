package require vtkcommon

#
# Here you should pick the name of one your common local classes
# instead of vtkBar.
#

if {[info commands vtkBar] != "" ||
    [::vtk::load_component vtkmyCommonTCL] == ""} {
    package provide vtkmycommon 4.0
}
