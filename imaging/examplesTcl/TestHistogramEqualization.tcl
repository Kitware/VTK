catch {load vtktcl}
# Developed By Majeid Alyassin
set slicenumber 0
set numslices  10
set xdim 256
set ydim 256

#set prefix "/home/alyassin2/database/gems/CTangio/CW1/LR_Data/max/i7903CTGE_sub"
#set prefix "/home/alyassin2/database/gems/CTangio/CW1/HR_Data/i7903CTGE_flat"
#set prefix "/home/alyassin2/database/gems/CTangio/CW1/processed/PM1_MaxT1390"
set prefix "../../../data/fullHead/headsq"

set numberofbins $xdim
set offsetlevel 5
set window 3000
set level  1500

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
	reader SetDataByteOrderToLittleEndian
	reader SetDataDimensions $xdim $ydim $numslices 1
	reader SetFilePrefix $prefix
	reader SetDataMask 0x7fff
	reader ReleaseDataFlagOff

vtkImageHistogram hist1
	#hist1 DebugOn
	hist1 SetNumberOfBins $numberofbins
	hist1 OffsetOn
	hist1 SetOffsetLevel $offsetlevel
	hist1 SetInput [reader GetOutput]
	hist1 ReleaseDataFlagOff

vtkImageHistogramEqualization histequal
	#histequal DebugOn
	histequal SetAveragingRadius 1
	histequal SetInput [reader GetOutput]
	histequal ReleaseDataFlagOff

vtkImageHistogram hist2
	#he DebugOn
	hist2 SetNumberOfBins $numberofbins
	hist2 OffsetOn
	hist2 SetOffsetLevel $offsetlevel
	hist2 SetInput [histequal GetOutput]
	hist2 ReleaseDataFlagOff

vtkImageViewer viewer3
	viewer3 SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
	viewer3 SetInput [hist2 GetOutput]
	viewer3 SetCoordinate2 $slicenumber
	viewer3 SetColorWindow $window
	viewer3 SetColorLevel $level
	viewer3 SetXOffset $xdim
	viewer3 SetYOffset $ydim;	
	viewer3 Render

vtkImageViewer viewer
	#viewer DebugOn
	viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
	viewer SetInput [reader GetOutput]
	viewer SetCoordinate2 $slicenumber
	viewer SetColorWindow $window
	viewer SetColorLevel  $level
	viewer SetXOffset 0
	viewer SetWindow [viewer3 GetWindow]

vtkImageViewer viewer1
	viewer1 SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
	viewer1 SetInput [histequal GetOutput]
	viewer1 SetCoordinate2 $slicenumber
	viewer1 SetColorWindow $window
	viewer1 SetColorLevel $level
	viewer1 SetXOffset $xdim
	viewer1 SetWindow [viewer3 GetWindow]

vtkImageViewer viewer2
	viewer2 SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
	viewer2 SetInput [hist1 GetOutput]
	viewer2 SetCoordinate2 $slicenumber
	viewer2 SetColorWindow $window
	viewer2 SetColorLevel $level
	viewer2 SetXOffset 0
	viewer2 SetYOffset $xdim
	viewer2 SetWindow [viewer3 GetWindow]

#make interface
#

frame .slice
button .slice.up -text "Slice Up" -command SliceUp
button .slice.down -text "Slice Down" -command SliceDown
entry  .slice.snum  -width 4 

frame .wl
frame .wl.f1
label .wl.f1.windowLabel -text "Window.....:"
scale .wl.f1.window -from 1 -to 3000 -length 5c -orient horizontal -command SetWindow
frame .wl.f2
label .wl.f2.levelLabel -text "Level.......:"
scale .wl.f2.level -from 1 -to 3000 -length 5c -orient horizontal -command SetLevel

frame .wl.f3
label .wl.f3.numofbinsLabel -text "# of Bins"
scale .wl.f3.numofbins -from 0 -to $numberofbins  -length 5c -orient horizontal -command SetNumofbins

frame .wl.f4
label .wl.f4.offsetlabel -text "Offset Level"
scale .wl.f4.leveloffset -from 0 -to 3000  -length 5c -orient horizontal -command Setoffsetlevel

pack .slice .wl -side left
pack .slice.up .slice.snum .slice.down -side top
pack .wl.f1 .wl.f2 .wl.f3 .wl.f4 -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left
pack .wl.f3.numofbinsLabel .wl.f3.numofbins -side left
pack .wl.f4.offsetlabel .wl.f4.leveloffset -side left

.wl.f1.window set  $window
.wl.f2.level set $level
.wl.f3.numofbins set $numberofbins
.wl.f4.leveloffset set $offsetlevel
.slice.snum   insert 0 $slicenumber
bind .slice.snum <Return> { SetSlice }

proc SliceUp {} {
   global slicenumber viewer viewer1 viewer2 viewer3 numslices 
   puts [expr $numslices-1]
   if {$slicenumber<[expr $numslices-1]} {set slicenumber [expr $slicenumber+1]}
   puts $slicenumber
   .slice.snum delete 0 10
   .slice.snum insert 0 $slicenumber
   viewer SetCoordinate2 $slicenumber
   viewer Render
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render
   viewer2 SetCoordinate2 $slicenumber
   viewer2 Render
   viewer3 SetCoordinate2 $slicenumber
   viewer3 Render
}

proc SliceDown {} {
   global slicenumber viewer numslices viewer1 viewer2 viewer3   
   puts [expr $numslices-1]
   if {$slicenumber > 0} {set slicenumber [expr $slicenumber - 1]}
   puts $slicenumber
   .slice.snum delete 0 10
   .slice.snum insert 0 $slicenumber
   viewer SetCoordinate2 $slicenumber
   viewer Render
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render
   viewer2 SetCoordinate2 $slicenumber
   viewer2 Render
   viewer3 SetCoordinate2 $slicenumber
   viewer3 Render

}
proc SetSlice {} {
   global slicenumber viewer numslices viewer1 viewer2 viewer3
   set slicenumber [.slice.snum  get]
   if {$slicenumber > [expr $numslices-1]} {set slicenumber [expr $numslices-1]}
   puts  $slicenumber
   viewer SetCoordinate2 $slicenumber
   viewer Render
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render
   viewer2 SetCoordinate2 $slicenumber
   viewer2 Render
   viewer3 SetCoordinate2 $slicenumber
   viewer3 Render
}

proc SetWindow window {
   global viewer viewer1 
   viewer SetColorWindow $window
   viewer Render
   viewer1 SetColorWindow $window
   viewer1 Render
}

proc SetLevel level {
   global viewer viewer1 
   viewer SetColorLevel $level
   viewer Render
   viewer1 SetColorLevel $level
   viewer1 Render

}
proc SetNumofbins numbofbins {
   global numberofbins viewer viewer1 viewer2 viewer3 hist1 hist2
   set numberofbins $numbofbins
   hist1 SetNumberOfBins $numbofbins
   hist2 SetNumberOfBins $numbofbins
   viewer Render
   viewer1 Render
   viewer2 Render
   viewer3 Render
}

proc Setoffsetlevel leveloffset {
   global offsetlevel viewer viewer1 viewer2 viewer3 hist1 hist2
   set offsetlevel $leveloffset
   hist1 SetOffsetLevel $offsetlevel
   hist2 SetOffsetLevel $offsetlevel
   viewer Render
   viewer1 Render
   viewer2 Render
   viewer3 Render
}



#$renWin Render
#wm withdraw









