catch {load vtktcl}
# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.


set sliceNumber 22

set VTK_FLOAT              1
set VTK_INT                2
set VTK_SHORT              3
set VTK_UNSIGNED_SHORT     4
set VTK_UNSIGNED_CHAR      5

set VTK_IMAGE_X_AXIS             0
set VTK_IMAGE_Y_AXIS             1
set VTK_IMAGE_Z_AXIS             2
set VTK_IMAGE_TIME_AXIS          3
set VTK_IMAGE_COMPONENT_AXIS     4



# Image pipeline

vtkImageSeriesReader reader
reader SwapBytesOn
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetPixelMask 0x7fff
reader SetOutputScalarType $VTK_SHORT
#reader DebugOn

vtkImageThreshold thresh
thresh SetInput [reader GetOutput]
thresh SetOutputScalarType $VTK_UNSIGNED_CHAR
thresh ThresholdByUpper 2000.0
thresh SetInValue 255
thresh SetOutValue 0
thresh ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
viewer SetInput [thresh GetOutput]
viewer SetExtent 0 255 0 255
viewer SetCoordinate2 $sliceNumber
viewer SetColorWindow 255
viewer SetColorLevel 128
#viewer DebugOn
viewer Render


#make interface
#

frame .slice
button .slice.up -text "Slice Up" -command SliceUp
button .slice.down -text "Slice Down" -command SliceDown

frame .wl
frame .wl.f1
label .wl.f1.windowLabel -text Window
scale .wl.f1.window -from 1 -to 300 -orient horizontal -command SetWindow
frame .wl.f2
label .wl.f2.levelLabel -text Level
scale .wl.f2.level -from 1 -to 150 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo


.wl.f1.window set 256
.wl.f2.level set 128


pack .slice .wl -side left
pack .slice.up .slice.down -side top
pack .wl.f1 .wl.f2 .wl.video -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left


proc SliceUp {} {
   global sliceNumber viewer
   if {$sliceNumber < 92} {set sliceNumber [expr $sliceNumber + 1]}
   puts $sliceNumber
   viewer SetCoordinate2 $sliceNumber
   viewer Render
}

proc SliceDown {} {
   global sliceNumber viewer
   if {$sliceNumber > 0} {set sliceNumber [expr $sliceNumber - 1]}
   puts $sliceNumber
   viewer SetCoordinate2 $sliceNumber
   viewer Render
}

proc SetWindow window {
   global viewer
   viewer SetColorWindow $window
   viewer Render
}

proc SetLevel level {
   global viewer
   viewer SetColorLevel $level
   viewer Render
}

proc SetInverseVideo {} {
   global viewer
   if { $inverseVideo == 0 } {
      viewer SetWindow -255
   } else {
      viewer SetWindow 255
   }		
   viewer Render
}


puts "Done"


#$renWin Render
#wm withdraw .








