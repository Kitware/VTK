# Convert a structured points data set into an image.
# Display the image in an XViewer.


set sliceNumber 14

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




vtkVolume16Reader v16
    v16 SetDataDimensions 256 256
    v16 SetFilePrefix "../data/fullHead/headsq";
    v16 SetDataOrigin -127.5 -127.5 5.33
    v16 SetImageRange 1 94
    v16 SetDataAspectRatio 1 1 5.33
    v16 SwapBytesOn
    v16 Update

vtkStructuredPointsToImage sti
sti SetInput [v16 GetOutput]

puts [sti Print]


#vtkImageAnisotropicDiffusion2d diffusion;
#diffusion SetInput [sti GetOutput];
#diffusion SetDiffusionFactor 0.15;
#diffusion SetDiffusionThreshold 20.0;
#diffusion SetNumberOfIterations 10;
#diffusion ReleaseDataFlagOff;


vtkImageXViewer viewer;
#viewer DebugOn;
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS;
viewer SetInput [sti GetOutput];
viewer SetCoordinate2 $sliceNumber;
viewer SetColorWindow 3000
viewer SetColorLevel 1500
viewer Render;


#make interface
#

frame .slice
button .slice.up -text "Slice Up" -command SliceUp
button .slice.down -text "Slice Down" -command SliceDown

frame .wl
frame .wl.f1;
label .wl.f1.windowLabel -text Window;
scale .wl.f1.window -from 1 -to 4000 -orient horizontal -command SetWindow
frame .wl.f2;
label .wl.f2.levelLabel -text Level;
scale .wl.f2.level -from 1 -to 2000 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo


.wl.f1.window set 3000
.wl.f2.level set 1500


pack .slice .wl -side left
pack .slice.up .slice.down -side top
pack .wl.f1 .wl.f2 .wl.video -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left


proc SliceUp {} {
   global sliceNumber viewer
   if {$sliceNumber < 93} {set sliceNumber [expr $sliceNumber + 1]}
   puts $sliceNumber
   viewer SetCoordinate2 $sliceNumber;
   viewer Render;
}

proc SliceDown {} {
   global sliceNumber viewer
   if {$sliceNumber > 0} {set sliceNumber [expr $sliceNumber - 1]}
   puts $sliceNumber
   viewer SetCoordinate2 $sliceNumber;
   viewer Render;
}

proc SetWindow window {
   global viewer
   viewer SetColorWindow $window;
   viewer Render;
}

proc SetLevel level {
   global viewer
   viewer SetColorLevel $level;
   viewer Render;
}

proc SetInverseVideo {} {
   global viewer
   if { $inverseVideo == 0 } {
      viewer SetWindow -255;
   } else {
      viewer SetWindow 255;
   }		
   viewer Render;
}


puts "Done";


#$renWin Render
#wm withdraw .








