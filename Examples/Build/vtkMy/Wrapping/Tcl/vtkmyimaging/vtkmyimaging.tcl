package require vtkmycommon
package require vtkimaging

#
# Here you should pick the name of one your imaging local classes
# instead of vtkImageFoo.
#

if {[info commands vtkImageFoo] != "" ||
    [::vtk::load_component vtkmyImagingTCL] == ""} {
    package provide vtkmyimaging 4.0
}
