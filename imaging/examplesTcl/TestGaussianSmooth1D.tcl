catch {load vtktcl}
# This script is for testing the 1d Gaussian Smooth filter.

source vtkImageInclude.tcl

set sliceNumber 22


# Image pipeline

vtkImageVolume16Reader reader
#reader DebugOn
reader SetDataByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetDataMask 0x7fff

vtkImageGaussianSmooth1D smooth
smooth SetInput [reader GetOutput]
smooth SetStride 2
smooth SetStandardDeviation 6
smooth SetFilteredAxis $VTK_IMAGE_Y_AXIS
smooth SetRadiusFactor 1.5
smooth ReleaseDataFlagOff

vtkImageViewer viewer
#viewer DebugOn
viewer SetInput [smooth GetOutput]
viewer SetZSlice $sliceNumber
viewer SetColorWindow 1200
viewer SetColorLevel 600
viewer SetOriginLocationToUpperLeft
viewer Render


#make interface
#

frame .slice
button .slice.up -text "Slice Up" -command SliceUp
button .slice.down -text "Slice Down" -command SliceDown

frame .wl
frame .wl.f1
label .wl.f1.windowLabel -text Window
scale .wl.f1.window -from 1 -to 3000 -orient horizontal -command SetWindow \
  -variable window
frame .wl.f2
label .wl.f2.levelLabel -text Level
scale .wl.f2.level -from 1 -to 1500 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -command SetInverseVideo


.wl.f1.window set 1200
.wl.f2.level set 600


pack .slice .wl -side left
pack .slice.up .slice.down -side top
pack .wl.f1 .wl.f2 .wl.video -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left


proc SliceUp {} {
   global sliceNumber viewer
   if {$sliceNumber < 92} {set sliceNumber [expr $sliceNumber + 1]}
   puts $sliceNumber
   viewer SetZSlice $sliceNumber
   viewer Render
}

proc SliceDown {} {
   global sliceNumber viewer
   if {$sliceNumber > 0} {set sliceNumber [expr $sliceNumber - 1]}
   puts $sliceNumber
   viewer SetZSlice $sliceNumber
   viewer Render
}

proc SetWindow window {
   global viewer video
   if {$video} {
      viewer SetColorWindow [expr -$window]
   } else {
      viewer SetColorWindow $window
   }
   viewer Render
}

proc SetLevel level {
   global viewer
   viewer SetColorLevel $level
   viewer Render
}

proc SetInverseVideo {} {
   global viewer video window
   if {$video} {
      viewer SetColorWindow [expr -$window]
   } else {
      viewer SetColorWindow $window
   }
   viewer Render
}








