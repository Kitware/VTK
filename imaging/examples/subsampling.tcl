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
	reader SetDimensions 512 512 30 1;
	#reader SetFilePrefix "/home/alyassin2/database/gems/CTangio/CW1/LR_Data/max/i7903CTGE_sub";
	reader SetFilePrefix "/home/alyassin2/database/gems/CTangio/CW1/original/i7903CTGE";
	#reader SetFilePrefix "/home/alyassin2/tks/old_vtk/stk/tcl/src/testimages/testfile";
	reader SetPixelMask 0x7fff;

vtkImageSubSampling ss;
	ss  MinimumOn;
	ss  SetSamplingFactors 2 2 1;
	ss  SetInput [reader GetOutput];

vtkImageRegion region;
	region SetExtent 0 255 0 255 0 29;
	[ss GetOutput] UpdateRegion region;

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
.wl.f1.window set 1
.wl.f2.level set 1

frame  .flag;
label  .flag.l1 -text "SubSampling With:";
radiobutton .flag.minflag -text "Min" -value 0 -command SetMinimumFlag;
radiobutton .flag.maxflag -text "Max"  -value 1 -command SetMaximumFlag;
radiobutton .flag.meanflag -text "Mean"  -value 2 -command SetMeanFlag;
radiobutton .flag.medianflag -text "Median" -value 3 -command SetMedianFlag;

pack .slice .wl .flag -side bottom
pack .slice.up .slice.down -side right
pack .wl.f1 .wl.f2  
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left
pack .flag.l1  
pack .flag.minflag .flag.maxflag .flag.meanflag .flag.medianflag -side left

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
proc SetMinimumFlag {} {
   global viewer
   global region
   ss MinimumOn; ss MaximumOff; ss MeanOff;ss MedianOff;
   [ss GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMaximumFlag {} {
   global viewer
   global region
   ss MinimumOff; ss  MaximumOn; ss MeanOff; ss MedianOff;
   [ss GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMeanFlag {} {
   global viewer
   global region
   ss MinimumOff; ss MaximumOff; ss MeanOn; ss MedianOff;
   [ss GetOutput] UpdateRegion region;
   viewer Render;
}
proc SetMedianFlag {} {
   global viewer
   global region
   ss MinimumOff; ss MaximumOff; ss MeanOff; ss MedianOn;
   [ss GetOutput] UpdateRegion region;
   viewer Render;
}


puts "Done";


#$renWin Render
#wm withdraw .








