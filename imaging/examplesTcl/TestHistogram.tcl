catch {load vtktcl}
# Developed By Majeid Alyassin


set firstslice 1
set slicenumber 0
set numslices  10
set xdim 256
set ydim 256

#set prefix "/home/alyassin2/database/gems/CTangio/CW1/HR_Data/i7903CTGE_flat"
set prefix "/home/alyassin2/database/Duke/ss_dualpanc_3150/ss_dualpanc"

#set prefix "../../../data/fullHead/headsq"
set numberofbins $xdim
set offsetlevel 1050
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

vtkImageVolume16Reader reader
	#reader DebugOn
	#reader SetDataByteOrderToLittleEndian
	reader SetDataDimensions $xdim $ydim $numslices 1
	reader SetFilePrefix $prefix
	reader SetDataMask 0x7fff
	reader ReleaseDataFlagOff

vtkImageHistogram hist
	#he DebugOn
	hist SetNumberOfBins $numberofbins
	hist OffsetOn
	hist SetOffsetLevel $offsetlevel
	hist SetInput [reader GetOutput]
	hist ReleaseDataFlagOff

vtkImageViewer viewer
	#viewer DebugOn
	viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
	viewer SetInput [reader GetOutput]
	viewer SetCoordinate2 $slicenumber
	viewer SetColorWindow $window
	viewer SetColorLevel $level
	viewer SetXOffset $xdim
	viewer Render

vtkImageViewer viewer1
	viewer1 SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_Z_AXIS
	viewer1 SetInput [hist GetOutput]
	viewer1 SetCoordinate2 $slicenumber
	viewer1 SetColorWindow $window
	viewer1 SetColorLevel $level
	viewer1 SetXOffset 0
	viewer1 SetWindow [viewer GetWindow]

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
scale .wl.f3.numofbins -from 0 -to $xdim  -length 5c -orient horizontal -command SetNumofbins

frame .wl.f4
label .wl.f4.offsetlabel -text "Offset Level"
scale .wl.f4.leveloffset -from 0 -to 3000  -length 5c -orient horizontal -command Setoffsetlevel

pack .slice .wl -side left
pack .slice.up .slice.snum .slice.down -side top
pack .wl.f1 .wl.f2 .wl.f3  .wl.f4 -side top
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
   global slicenumber viewer viewer1 numslices 
   puts [expr $numslices-1]
   if {$slicenumber<[expr $numslices-1]} {set slicenumber [expr $slicenumber+1]}
   puts $slicenumber
   .slice.snum delete 0 10
   .slice.snum insert 0 $slicenumber
   viewer SetCoordinate2 $slicenumber
   viewer Render
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render
}

proc SliceDown {} {
   global slicenumber viewer numslices viewer1    
   puts [expr $numslices-1]
   if {$slicenumber > 0} {set slicenumber [expr $slicenumber - 1]}
   puts $slicenumber
   .slice.snum delete 0 10
   .slice.snum insert 0 $slicenumber
   viewer SetCoordinate2 $slicenumber
   viewer Render
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render

}
proc SetSlice {} {
   global slicenumber viewer numslices viewer1 
   set slicenumber [.slice.snum  get]
   if {$slicenumber > [expr $numslices-1]} {set slicenumber [expr $numslices-1]}
   puts  $slicenumber
   viewer SetCoordinate2 $slicenumber
   viewer Render
   viewer1 SetCoordinate2 $slicenumber
   viewer1 Render
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
   global numberofbins viewer1 hist 
   set numberofbins $numbofbins
   hist SetNumberOfBins $numbofbins
   viewer1 Render

}

proc Setoffsetlevel leveloffset {
   global offsetlevel  viewer1 hist
   set offsetlevel $leveloffset
   hist SetOffsetLevel $offsetlevel
   viewer1 Render
}




#$renWin Render
#wm withdraw









