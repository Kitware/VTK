# Test the new reader


source vtkImageInclude.tcl


# Image pipeline

vtkImageFileReader reader;
reader ReleaseDataFlagOff;
reader SetAxes $VTK_IMAGE_COMPONENT_AXIS $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS 
reader SetDataDimensions 3 512 256;
reader SetFileName "../../data/earth.ppm";
reader SetInputScalarType $VTK_UNSIGNED_CHAR;
#reader DebugOn


vtkImageXViewer viewer;
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS;
viewer SetInput [reader GetOutput];
viewer SetColorWindow 256
viewer SetColorLevel 128
viewer ColorFlagOn;
#viewer DebugOn;
viewer Render;


#make interface
#

frame .wl
frame .wl.f1;
label .wl.f1.windowLabel -text Window;
scale .wl.f1.window -from 1 -to 300 -orient horizontal -command SetWindow
frame .wl.f2;
label .wl.f2.levelLabel -text Level;
scale .wl.f2.level -from 1 -to 150 -orient horizontal -command SetLevel
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








