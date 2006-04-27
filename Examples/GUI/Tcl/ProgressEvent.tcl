# Halves the size of the image in the x, y and z dimensions.
package require vtk

# Image pipeline
vtkImageReader reader
  reader SetDataByteOrderToLittleEndian
  reader SetDataExtent 0 255 0 255 1 93
  reader SetFilePrefix $VTK_DATA_ROOT/Data/headsq/quarter
  reader SetDataMask 0x7fff

vtkImageShrink3D shrink
  shrink SetInputConnection [reader GetOutputPort]
  shrink SetShrinkFactors 2 2 2
  shrink AddObserver ProgressEvent {
    .text configure -text "Completed [expr [shrink GetProgress]*100.0] percent"
    update
  }
  shrink AddObserver EndEvent {
    .text configure -text "Completed Processing"
    update
  }

button .run -text "Execute" -command {
  shrink Modified
  shrink Update
}
label .text -text "Waiting to Process"
pack .run .text
