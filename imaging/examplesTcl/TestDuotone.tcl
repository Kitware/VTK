# Test the object vtkImagePaint which is a region that has methods to draw
# lines and Boxs in different colors.


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



# Create a color image to test the filter
vtkImagePaint canvas;
canvas SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS 
canvas SetScalarType $VTK_FLOAT
canvas SetExtent 0 511 0 511 0 2;
canvas SetDrawColor 100 100 0;
canvas FillBox 0 511 0 511;
canvas SetDrawColor 200 0 200;
canvas FillBox 32 511 100 500;
canvas SetDrawColor 100 0 0;
canvas FillTube 500 20 30 400 5;
canvas SetDrawColor 255 255 255;
canvas DrawSegment 10 20 90 510;
canvas SetDrawColor 200 50 50;
canvas DrawSegment 510 90 10 20;
canvas SetDrawColor 0 200 0;
canvas DrawSegment -10 30 30 -10;
canvas DrawSegment -10 481 30 521;
canvas DrawSegment 481 -10 521 30;
canvas DrawSegment 481 521 521 481;
canvas SetDrawColor 20 200 200;
canvas FillTriangle 100 100  300 150  150 300;
canvas SetDrawColor 250 250 10;
canvas DrawCircle 350 350  200.0;
canvas SetDrawColor 250 250 250;
canvas DrawPoint 350 350;

canvas SetDrawColor 0 255 255;
canvas FillBox 0 50 0 50;
canvas SetDrawColor 0 255 150;
canvas FillBox 50 100 0 50;
canvas SetDrawColor 50 255 50;
canvas FillBox 100 150 0 50;
canvas SetDrawColor 150 255 0;
canvas FillBox 150 200 0 50;
canvas SetDrawColor 255 255 0;
canvas FillBox 200 250 0 50;

# split the image into two components
vtkImageDuotone duotone
duotone SetInput [canvas GetOutput];
duotone SetInk0 0 0 0;
duotone SetInk1 0 255 55;
duotone SetOutputMaximum 255;
duotone SetOutputMaximum 255;
duotone ReleaseDataFlagOff;

vtkImageRDuotone rd;
rd SetInput1 [duotone GetOutput0];
rd SetInput2 [duotone GetOutput1];
rd SetInk0 0 0 0;
rd SetInk1 0 255 55;
rd SetOutputMaximum 255;
rd SetOutputMaximum 255;
rd ReleaseDataFlagOff;

vtkImageXViewer viewerR;
viewerR SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS;
viewerR SetInput [rd GetOutput];
viewerR SetColorWindow 256
viewerR SetColorLevel 128
viewerR ColorFlagOn;
viewerR SetXOffset 512;
viewerR Render;

vtkImageXViewer viewerD;
viewerD SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS;
viewerD SetInput [canvas GetOutput];
viewerD SetColorWindow 256
viewerD SetColorLevel 128
viewerD ColorFlagOn;
viewerD SetWindow [viewerR GetWindow];
viewerD Render;





vtkImageXViewer viewer0;
viewer0 SetInput [duotone GetOutput0];
viewer0 SetColorWindow 256
viewer0 SetColorLevel 128
viewer0 SetXOffset 512;
viewer0 Render;

vtkImageXViewer viewer1;
viewer1 SetInput [duotone GetOutput1];
viewer1 SetColorWindow 256
viewer1 SetColorLevel 128
viewer1 SetWindow [viewer0 GetWindow];
viewer1 Render;


#make interface
#

frame .wl
frame .wl.f1;
label .wl.f1.windowLabel -text Window;
scale .wl.f1.window -from 1 -to 400 -orient horizontal -command SetWindow
frame .wl.f2;
label .wl.f2.levelLabel -text Level;
scale .wl.f2.level -from -200 -to 200 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo


.wl.f1.window set 256
.wl.f2.level set 128


pack .wl -side left
pack .wl.f1 .wl.f2 .wl.video -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left



proc SetWindow window {
   global viewerD viewerR
   viewerD SetColorWindow $window;
   viewerR SetColorWindow $window;
   viewerD Render;
   viewerR Render;
}

proc SetLevel level {
   global viewerD viewerR
   global viewer
   viewerD SetColorLevel $level;
   viewerR SetColorLevel $level;
   viewerD Render;
   viewerR Render;
}

proc SetInverseVideo {} {
}


puts "Done";


#$renWin Render
#wm withdraw .








