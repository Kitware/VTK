catch {load vtktcl}
# Simple viewer for images.

set firstslice 1
set slicenumber 5
set numslices  30
set prefix "/home/alyassin2/database/gems/CTangio/CW1/LR_Data/max/i7903CTGE_sub"

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
        reader DebugOn
	#reader SetDataByteOrderToBigEndian
	reader SetDataDimensions 128 128 $numslices 1
        reader  SetFirst $firstslice
	reader SetFilePrefix $prefix 
	reader SetDataMask 0x7fff
	reader ReleaseDataFlagOff

vtkImageConnectivity connect
	connect PercentLevelValueOn
	connect SetPLevelSeedValue 0.9
	connect SetNeighbors 26
	connect SetThreshold 1350
	#connect SingleSeedOn
	#connect SetSeedXYZ 17 61 5
	connect SetOutputScalarType $VTK_UNSIGNED_CHAR
	connect SetInput [reader GetOutput]
	connect ReleaseDataFlagOff

vtkImageMarkBoundary mb
	mb SetDilateValue 1
	mb SetSurfaceValue 100
	mb SetKernelRadius 1
	mb SetOutputScalarType $VTK_UNSIGNED_CHAR
	mb SetInput [connect GetOutput]
	mb ReleaseDataFlagOff

vtkImageRegion region
	region SetExtent 0 127 0 127 0 [expr $numslices -1]
	[mb GetOutput] UpdateRegion region

vtkImageViewer viewer
	#viewer DebugOn
	viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
	viewer SetInput [region GetOutput]
	viewer SetCoordinate2 $slicenumber
	viewer SetColorWindow 1
	viewer SetColorLevel  1
	viewer Render


#make interface
#

frame .slice
button .slice.up -text "Slice Up" -command SliceUp
button .slice.down -text "Slice Down" -command SliceDown
entry  .slice.snum  -width 4 

frame .wl
frame .wl.f1
label .wl.f1.windowLabel -text "Window.....:"
scale .wl.f1.window -from 1 -to 200 -length 5c -orient horizontal -command SetWindow
frame .wl.f2
label .wl.f2.levelLabel -text "Level.......:"
scale .wl.f2.level -from 1 -to 200 -length 5c -orient horizontal -command SetLevel

frame .wl.f3
label .wl.f3.stLabel -text "MB Radius:"
scale .wl.f3.st -from 0 -to 20 -length 5c -orient horizontal -command SetST


pack .slice .wl -side left
pack .slice.up .slice.snum .slice.down -side top
pack .wl.f1 .wl.f2 .wl.f3 -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left
pack .wl.f3.stLabel .wl.f3.st -side left

global slicenumber
.wl.f1.window set 1
.wl.f2.level  set 1
.wl.f3.st     set 1
.slice.snum   insert 0 $slicenumber
bind .slice.snum <Return> { SetSlice }

proc SliceUp {} {
   global slicenumber viewer numslices
   puts [expr $numslices-1]
   if {$slicenumber<[expr $numslices-1]} {set slicenumber [expr $slicenumber+1]}
   puts $slicenumber
   .slice.snum delete 0 10
   .slice.snum insert 0 $slicenumber
   viewer SetCoordinate2 $slicenumber
   viewer Render
}

proc SliceDown {} {
   global slicenumber viewer numslices
   puts [expr $numslices-1]
   if {$slicenumber > 0} {set slicenumber [expr $slicenumber - 1]}
   puts $slicenumber
   .slice.snum delete 0 10
   .slice.snum insert 0 $slicenumber
   viewer SetCoordinate2 $slicenumber
   viewer Render
}
proc SetSlice {} {
   global slicenumber viewer numslices 
   set slicenumber [.slice.snum  get]
   if {$slicenumber > [expr $numslices-1]} {set slicenumber [expr $numslices-1]}
   puts  $slicenumber
   viewer SetCoordinate2 $slicenumber
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
proc SetST st {
   global viewer mb region 
   mb SetKernelRadius [.wl.f3.st get]
   [mb GetOutput] UpdateRegion region
   viewer Render
}











