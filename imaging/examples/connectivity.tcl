# Simple viewer for images.


set sliceNumber 5

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
	reader ReleaseDataFlagOff;
	reader SwapBytesOff;
	reader SetDimensions 128 128 60 1;
	reader SetFilePrefix "/home/alyassin2/database/gems/CTangio/CW1/LR_Data/max/i7903CTGE_sub";
	reader SetPixelMask 0x7fff;

vtkImageConnectivity connect;
	connect PercentLevelValueOn;
	connect SetPLevelSeedValue 0.9;
	connect SetNeighbors 26;
	connect SetThreshold 1350;
	#connect SingleSeedOn;
	#connect SetSeedXYZ 17 61 5;
	connect SetInput [reader GetOutput];
	connect ReleaseDataFlagOff;


vtkImageRegion region;
	region SetExtent 0 127 0 127 0 59;
	[connect GetOutput] UpdateRegion region;


vtkImageXViewer viewer;
#viewer DebugOn;
	viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS;
	viewer SetInput [region GetOutput];
	viewer SetCoordinate2 $sliceNumber;
	viewer SetColorWindow 255
	viewer SetColorLevel 127
	viewer Render;


#make interface
#

frame .slice
button .slice.up -text "Slice Up" -command SliceUp
button .slice.down -text "Slice Down" -command SliceDown

frame .wl
frame .wl.f1;
label .wl.f1.windowLabel -text Window;
scale .wl.f1.window -from 1 -to 3000 -orient horizontal -command SetWindow
frame .wl.f2;
label .wl.f2.levelLabel -text Level;
scale .wl.f2.level -from 1 -to 1500 -orient horizontal -command SetLevel

frame .wl.f3;
label .wl.f3.plsvLabel -text LSV%;
scale .wl.f3.plsv -from 0 -to 101  -orient horizontal -command SetPlsv


.wl.f1.window set 1
.wl.f2.level set 1
.wl.f3.plsv set 1


pack .slice .wl -side left
pack .slice.up .slice.down -side top
pack .wl.f1 .wl.f2 .wl.f3 -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left
pack .wl.f3.plsvLabel .wl.f3.plsv -side left


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
proc SetPlsv plsv {
   set plsvlevel  [expr $plsv/100.0];
   puts $plsvlevel;
   global viewer
   global region
   connect SetPLevelSeedValue $plsvlevel;
   [connect GetOutput] UpdateRegion region;
   viewer Render;
}


puts "Done";


#$renWin Render
#wm withdraw









