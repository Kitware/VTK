catch {load vtktcl}
# This scripts performs bandpass in frequency domain.
# It demostrates the ability to stream even FFT filters.


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
reader SetFileByteOrderToLittleEndian
reader SetDataDimensions 256 256 93
reader SetFilePrefix "../../../data/fullHead/headsq"
reader SetPixelMask 0x7fff
reader SetOutputScalarType $VTK_SHORT
reader DebugOn

vtkImageFFT fft
fft SetDimensionality 2
fft SetInput [reader GetOutput]
fft SetInputMemoryLimit 150
fft DebugOn

vtkImageFourierBandPass bandPass
bandPass SetInput [fft GetOutput]
bandPass SetLowPass 0.1 0.1
bandPass SetHighPass 0.4 0.4
bandPass ReleaseDataFlagOff
bandPass SetInputMemoryLimit 150
bandPass DebugOn

vtkImageRFFT rfft
rfft SetDimensionality 2
rfft SetInput [bandPass GetOutput]
rfft ReleaseDataFlagOff
rfft SetInputMemoryLimit 150
rfft SetOutputScalarType $VTK_SHORT
rfft DebugOn



vtkImageViewer viewer
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
viewer SetInput [rfft GetOutput]
viewer SetCoordinate2 $sliceNumber
#viewer SetCoordinate3 0
viewer SetColorWindow 1600
viewer SetColorLevel 600
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
scale .wl.f1.window -from 1 -to 4000 -orient horizontal -command SetWindow
frame .wl.f2
label .wl.f2.levelLabel -text Level
scale .wl.f2.level -from 1 -to 2000 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo


.wl.f1.window set 1600
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





#$renWin Render
#wm withdraw .








