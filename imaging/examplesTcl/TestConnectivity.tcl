catch {load vtktcl}
catch {load vtktcl}
# Developed By Majeid Alyassin


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
#reader DebugOn
	#reader SwapBytesOff
	reader SetDataDimensions 128 128 60 1
	reader SetFilePrefix $prefix
	reader SetPixelMask 0x7fff
	reader ReleaseDataFlagOff

vtkImageConnectivity connect
	connect PercentLevelValueOn
	connect SetPLevelSeedValue 0.9
	connect SetNeighbors 26
	connect SetThreshold 1350
	#connect SingleSeedOn
	#connect SetSeedXYZ 17 61 5
	connect SetInput [reader GetOutput]
	connect ReleaseDataFlagOff


vtkImageRegion region
	region SetExtent 0 127 0 127 0 [expr $numslices-1]
	[connect GetOutput] UpdateRegion region


vtkImageXViewer viewer
#viewer DebugOn
	viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
	viewer SetInput [region GetOutput]
	viewer SetCoordinate2 $slicenumber
	viewer SetColorWindow 255
	viewer SetColorLevel 127
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
label .wl.f3.plsvLabel -text "% of Max:"
scale .wl.f3.plsv -from 0 -to 101  -length 5c -orient horizontal -command SetPlsv


.wl.f1.window set 1
.wl.f2.level set 1
.wl.f3.plsv set 1


pack .slice .wl -side left
pack .slice.up .slice.snum .slice.down -side top
pack .wl.f1 .wl.f2 .wl.f3 -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left
pack .wl.f3.plsvLabel .wl.f3.plsv -side left

global slicenumber
.wl.f1.window set 1
.wl.f2.level set 1
.wl.f3.plsv set 1
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
proc SetPlsv plsv {
   set plsvlevel  [expr $plsv/100.0]
   puts $plsvlevel
   global viewer
   global region
   connect SetPLevelSeedValue $plsvlevel
   [connect GetOutput] UpdateRegion region
   viewer Render
}


puts "Done"


#$renWin Render
#wm withdraw









