# Test the first annotation placement routine.


source vtkImageInclude.tcl


vtkImageAnnotate canvas;
canvas SetScalarType $VTK_UNSIGNED_CHAR
canvas SetExtent 0 949 0 749;
canvas SetDrawColor 0;
canvas FillBox 0 949 0 749;

# Check Filling a triangle
canvas SetDrawColor 50;
canvas FillBox 252 661 250 500;

canvas SetDrawColor 100;
canvas FillTube 510 190 190 420 10;

canvas SetDrawColor 150;
canvas FillTube 650 220 180 500 5;

canvas SetDrawColor 200;
canvas FillTriangle 400 200  600 250  450 400;

canvas SetDrawColor 225;
canvas FillTube 520 425 570 450 25;

canvas SetDrawColor 254;
canvas FillTube 200 220 410 510 10;

canvas ComputeBounds;
canvas Annotate 50;
canvas Annotate 100;
canvas Annotate 150;
canvas Annotate 200;
canvas Annotate 225;
canvas Annotate 254;

vtkImageXViewer viewer;
viewer SetInput [canvas GetOutput];
viewer SetColorWindow 256
viewer SetColorLevel 128
#viewer DebugOn;
viewer Render;


#make interface
#

frame .wl
frame .wl.f1;
label .wl.f1.windowLabel -text Window;
scale .wl.f1.window -from 1 -to 512 -orient horizontal -command SetWindow
frame .wl.f2;
label .wl.f2.levelLabel -text Level;
scale .wl.f2.level -from 1 -to 256 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo


.wl.f1.window set 256
.wl.f2.level set 128


pack .wl -side left
pack .wl.f1 .wl.f2 .wl.video -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left



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








