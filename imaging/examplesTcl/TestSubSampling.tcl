# Developed By Majeid Alyassin


set slicenumber 5
set ssflag 0
set numslices  30
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
set prefix "/home/alyassin2/database/gems/CTangio/CW1/original/i7903CTGE";
# Image pipeline

vtkImageShortReader reader;
#reader DebugOn
	reader ReleaseDataFlagOff;
	reader SwapBytesOff;
	reader SetDimensions 512 512 $numslices 1;
	reader SetFilePrefix $prefix;
	reader SetPixelMask 0x7fff;

vtkImageSubSampling ss;
        ss   MinimumOn;
	ss   SetSamplingFactors 4 4 1;
	ss   SetInput [reader GetOutput];

vtkImageRegion region;
	region SetExtent 0 127 0 127 0 29;
	[ss GetOutput] UpdateRegion region;

vtkImageXViewer viewer;
#viewer DebugOn;
	viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS;
	viewer SetInput [region GetOutput];
	viewer SetCoordinate2 $slicenumber;
	viewer SetColorWindow 255
	viewer SetColorLevel 127
	viewer Render;


#make interface
#

frame .slice
button .slice.up -text "Slice Up" -command SliceUp
button .slice.down -text "Slice Down" -command SliceDown
entry  .slice.snum  -width 4 

frame .wl
frame .wl.f1;
label .wl.f1.windowLabel -text "Window:";
scale .wl.f1.window -from 1 -to 2000 -length 5c -orient horizontal -command SetWindow
frame .wl.f2;
label .wl.f2.levelLabel -text "Level...";
scale .wl.f2.level -from 1 -to 3000 -length 5c -orient horizontal -command SetLevel
.wl.f1.window set 500
.wl.f2.level set 1100

frame  .flag;
label  .flag.l1 -text "SubSampling With:";
radiobutton .flag.minflag -text "Min" -value 0 -variable ssflag -command SetMinimumFlag;
radiobutton .flag.maxflag -text "Max"  -value 1 -variable ssflag -command SetMaximumFlag;
radiobutton .flag.meanflag -text "Mean"  -value 2 -variable ssflag -command SetMeanFlag;
radiobutton .flag.medianflag -text "Median" -value 3 -variable ssflag -command SetMedianFlag;

pack .slice .wl .flag -side bottom
pack .slice.up .slice.snum .slice.down -side right
pack .wl.f1 .wl.f2  
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left
pack .flag.l1  
pack .flag.minflag .flag.maxflag .flag.meanflag .flag.medianflag -side left

global slicenumber
.slice.snum insert 0 $slicenumber;
bind .slice.snum    <Return> { SetSlice }

proc SliceUp {} {
   global slicenumber viewer numslices
   puts [expr $numslices-1];
   if {$slicenumber < [expr $numslices-1]} {set slicenumber [expr $slicenumber + 1]}
   puts $slicenumber
   .slice.snum delete 0 10;
   .slice.snum insert 0 $slicenumber;
   viewer SetCoordinate2 $slicenumber;
   viewer Render;
}

proc SliceDown {} {
   global slicenumber viewer numslices
   puts [expr $numslices-1];
   if {$slicenumber > 0} {set slicenumber [expr $slicenumber - 1]}
   puts $slicenumber
   .slice.snum delete 0 10;
   .slice.snum insert 0 $slicenumber;
   viewer SetCoordinate2 $slicenumber;
   viewer Render;
}
proc SetSlice {} {
   global slicenumber viewer numslices 
   set slicenumber [.slice.snum  get];
   if {$slicenumber > [expr $numslices-1]} {set slicenumber [expr $numslices-1]}
   puts  $slicenumber;
   viewer SetCoordinate2 $slicenumber;
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







