catch {load vtktcl}
catch {load vtktcl}
# This script is for testing the 3d NonMaximumSuppressionFilter.
# The filter works exclusively on the output of the gradient filter.
# The effect is to pick the peaks of the gradient creating thin surfaces.


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
#reader DebugOn

vtkImageGradient gradient
gradient SetDimensionality 3
gradient SetInput [reader GetOutput]
gradient SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
gradient ReleaseDataFlagOff

vtkImageMagnitude magnitude
magnitude SetInput [gradient GetOutput]

vtkImageNonMaximumSuppression suppress
suppress SetVectorInput [gradient GetOutput]
suppress SetMagnitudeInput [magnitude GetOutput]
suppress SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
suppress ReleaseDataFlagOff

vtkImageXViewer viewer
viewer SetInput [suppress GetOutput]
viewer SetCoordinate2 $sliceNumber
viewer SetColorWindow 1000
viewer SetColorLevel 500
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
scale .wl.f1.window -from 1 -to 2000 -orient horizontal -command SetWindow
frame .wl.f2
label .wl.f2.levelLabel -text Level
scale .wl.f2.level -from 1 -to 1000 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo


.wl.f1.window set 1000
.wl.f2.level set 500


pack .slice .wl -side left
pack .slice.up .slice.down -side top
pack .wl.f1 .wl.f2 .wl.video -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left

#$renWin SetTkWindow .renwin

proc SliceUp {} {
        global sliceNumber viewer
        if {$sliceNumber < 92} {set sliceNumber [expr $sliceNumber + 1]}
        viewer SetCoordinate2 $sliceNumber
	viewer Render
        puts $sliceNumber
}

proc SliceDown {} {
        global sliceNumber tform renWin
        if {$sliceNumber > 0} {set sliceNumber [expr $sliceNumber - 1]}
        viewer SetCoordinate2 $sliceNumber
	viewer Render
        puts $sliceNumber
}

proc SetWindow window {
	global viewer
        viewer SetColorWindow $window
        viewer Render;  
}
proc SetLevel level {
	global viewer
        viewer SetColorLevel $level
        viewer Render;  
}
proc SetInverseVideo {} {
}







