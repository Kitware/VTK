# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.


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

vtkImageShortReader reader;
#reader DebugOn
reader SwapBytesOn;
reader SetDimensions 256 256 93;
reader SetFilePrefix "../../data/fullHead/headsq"
reader SetPixelMask 0x7fff;

vtkImageSobel3D sobel;
sobel SetInput [reader GetOutput];
sobel SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS;
sobel ReleaseDataFlagOff;





vtkImageXViewer viewer;
#viewer DebugOn;
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS $VTK_IMAGE_Z_AXIS;
viewer SetInput [sobel GetOutput];
viewer SetCoordinate3 $sliceNumber;
viewer SetColorWindow 200;
viewer SetColorLevel 0;
viewer ColorFlagOn;
viewer SetRed 0;
viewer SetGreen 1;
viewer SetBlue 2;
viewer Render;


#make interface
#

frame .slice
button .slice.up -text "Slice Up" -command SliceUp
button .slice.down -text "Slice Down" -command SliceDown

frame .wl
frame .wl.f1;
label .wl.f1.windowLabel -text Window;
scale .wl.f1.window -from 1 -to 1000 -orient horizontal -command SetWindow
frame .wl.f2;
label .wl.f2.levelLabel -text Level;
scale .wl.f2.level -from -1000 -to 1000 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo


.wl.f1.window set 200
.wl.f2.level set 0


pack .slice .wl -side left
pack .slice.up .slice.down -side top
pack .wl.f1 .wl.f2 .wl.video -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left


proc SliceUp {} {
   global sliceNumber viewer
   if {$sliceNumber < 92} {set sliceNumber [expr $sliceNumber + 1]}
   puts $sliceNumber
   viewer SetCoordinate3 $sliceNumber;
   viewer Render;
}

proc SliceDown {} {
   global sliceNumber viewer
   if {$sliceNumber > 0} {set sliceNumber [expr $sliceNumber - 1]}
   puts $sliceNumber
   viewer SetCoordinate3 $sliceNumber;
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








