catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# This is an example of how to extract slices from a volume.
# In this example, we allow vtkImageReslice to calculate the
# output origin for us, rather than explicitly calling
# reslice SetOutputOrigin.  This means that the output images
# will always be centered, and when used in combination with
# SetOutputDimensionality(2) the output slices will pass through
# whatever point we set as the ResliceAxesOrigin.

# This is meant as a quick-and-dirty example.  The corresponding
# python example, SliceViewerWidget.py, is developed to a much
# greater extent.

# Image pipeline

vtkImageReader reader
reader SetDataByteOrderToLittleEndian
reader SetDataExtent 0 255 0 255 1 93
reader SetDataSpacing 1.0 1.0 2.0
reader SetDataOrigin -128 -128 -92
reader SetFilePrefix "$VTK_DATA/fullHead/headsq"
reader SetDataMask 0x7fff
#reader DebugOn
reader Update

# we want to extract slices through the center of the volume
set center [[reader GetOutput] GetCenter]
set x [lindex $center 0]
set y [lindex $center 1]
set z [lindex $center 2]

# a transform can be applied to the image before slicing if desired
#  - uncomment in reslice1, reslice2 etc. to apply to the image
vtkTransform transform
# rotate about the center of the image
transform Translate $x $y $z
transform RotateWXYZ 10 1 1 0
transform Translate [expr - $x] [expr - $y] [expr - $z]
transform Inverse

# coronal slice
vtkImageReslice reslice1
reslice1 SetOutputDimensionality 2
reslice1 SetResliceAxesDirectionCosines +1 0 0  0 0 -1  0 +1 0
reslice1 SetResliceAxesOrigin           $x $y $z
#reslice1 SetResliceTransform transform
reslice1 SetInterpolationModeToLinear
reslice1 SetOutputSpacing 1 1 1
reslice1 SetOutputExtent 0 255 0 255 0 0
reslice1 SetInput [reader GetOutput]

# sagittal slice
vtkImageReslice reslice2
reslice2 SetOutputDimensionality 2
reslice2 SetResliceAxesDirectionCosines 0 +1 0  0 0 -1  -1 0 0
reslice2 SetResliceAxesOrigin           $x $y $z
#reslice2 SetResliceTransform transform
reslice2 SetInterpolationModeToLinear
reslice2 SetOutputSpacing 1 1 1
reslice2 SetOutputExtent 0 255 0 255 0 0
reslice2 SetInput [reader GetOutput]

# axial slice
vtkImageReslice reslice3
reslice3 SetOutputDimensionality 2
reslice3 SetResliceAxesDirectionCosines +1 0 0  0 +1 0  0 0 +1
reslice3 SetResliceAxesOrigin           $x $y $z
#reslice3 SetResliceTransform transform
reslice3 SetInterpolationModeToLinear
reslice3 SetOutputSpacing 1 1 1
reslice3 SetOutputExtent 0 255 0 255 0 0
reslice3 SetInput [reader GetOutput]

# oblique slice (you can manually set DirectionCosines and Origin,
# but using a transform to do it is easier)
vtkTransform oblique
# set origin (the 'fulcrum' for the rotation)
oblique Translate  $x $y $z
# set direction cosines
oblique RotateWXYZ 30  1 4 0

vtkImageReslice reslice4
reslice4 SetOutputDimensionality 2
reslice4 SetResliceAxes [oblique GetMatrix]
#reslice4 SetResliceTransform transform
reslice4 SetInterpolationModeToLinear
reslice4 SetOutputSpacing 1 1 1
reslice4 SetOutputExtent 0 255 0 255 0 0
reslice4 SetInput [reader GetOutput]

vtkImageMapper mapper1
  mapper1 SetInput [reslice1 GetOutput]
  mapper1 SetColorWindow 2000
  mapper1 SetColorLevel 1000
  mapper1 SetZSlice 0

vtkImageMapper mapper2
  mapper2 SetInput [reslice2 GetOutput]
  mapper2 SetColorWindow 2000
  mapper2 SetColorLevel 1000
  mapper2 SetZSlice 0 

vtkImageMapper mapper3
  mapper3 SetInput [reslice3 GetOutput]
  mapper3 SetColorWindow 2000
  mapper3 SetColorLevel 1000
  mapper3 SetZSlice 0 

vtkImageMapper mapper4
  mapper4 SetInput [reslice4 GetOutput]
  mapper4 SetColorWindow 2000
  mapper4 SetColorLevel 1000
  mapper4 SetZSlice 0 

vtkActor2D actor1
  actor1 SetMapper mapper1

vtkActor2D actor2
  actor2 SetMapper mapper2

vtkActor2D actor3
  actor3 SetMapper mapper3

vtkActor2D actor4
  actor4 SetMapper mapper4

vtkImager imager1
  imager1 AddActor2D actor1
  imager1 SetViewport 0.0 0.5 0.5 1.0

vtkImager imager2
  imager2 AddActor2D actor2
  imager2 SetViewport 0.5 0.5 1.0 1.0

vtkImager imager3
  imager3 AddActor2D actor3
  imager3 SetViewport 0.0 0.0 0.5 0.5

vtkImager imager4
  imager4 AddActor2D actor4
  imager4 SetViewport 0.5 0.0 1.0 0.5


vtkImageWindow imgWin
  imgWin EraseOff
  #imgWin DoubleBufferOn
  imgWin AddImager imager1
  imgWin AddImager imager2
  imgWin AddImager imager3
  imgWin AddImager imager4
  imgWin SetSize 512 512


#only use this interface when not doing regression tests
if {[info commands rtExMath] != "rtExMath"} {

proc InitializeInterface {} {
   global mapper1 mapper2 mapper3 mapper4
   global reslice1 reslice2 reslice3 reslice4 

   [reslice1 GetInput] UpdateInformation
   set center [[reslice1 GetInput] GetCenter]
   set bounds [[reslice1 GetInput] GetBounds]
   set spacing [[reslice1 GetInput] GetSpacing]

   set xl [lindex $bounds 0]
   set xh [lindex $bounds 1]
   set yl [lindex $bounds 2]
   set yh [lindex $bounds 3]
   set zl [lindex $bounds 4]
   set zh [lindex $bounds 5]

   set sx [lindex $spacing 0]
   set sy [lindex $spacing 1]
   set sz [lindex $spacing 2]

   set w [mapper1 GetColorWindow]
   set l [mapper1 GetColorLevel]

   frame .ct
   frame .ct.f1
   label .ct.f1.xLabel -text "X(mm)"
   scale .ct.f1.x -from $xl -to $xh  -orient horizontal \
	   -resolution $sx -command SetX -variable x
   frame .ct.f2
   label .ct.f2.yLabel -text "Y(mm)"
   scale .ct.f2.y -from $yl -to $yh  -orient horizontal \
	   -resolution $sy -command SetY -variable y
   frame .ct.f3
   label .ct.f3.zLabel -text "Z(mm)"
   scale .ct.f3.z -from $zl -to $zh  -orient horizontal \
	   -resolution $sz -command SetZ -variable z
   frame .ct.f4
   label .ct.f4.oLabel -text "O(mm)"
   scale .ct.f4.o -from $zl -to $zh  -orient horizontal \
	   -resolution 0.5 -command SetO -variable o
   frame .ct.f5
   label .ct.f5.sLabel -text "Spacing"
   scale .ct.f5.s -from 0.1 -to 2  -orient horizontal \
	   -resolution 0.01 -command SetSpacing -variable s

   .ct.f1.x set [lindex $center 0]
   .ct.f2.y set [lindex $center 1]
   .ct.f3.z set [lindex $center 2]
   .ct.f4.o set [lindex $center 2]
   .ct.f5.s set 1

   frame .wl
   frame .wl.f1
   label .wl.f1.windowLabel -text "Window"
   scale .wl.f1.window -from 1 -to [expr $w * 2]  -orient horizontal \
     -command SetWindow -variable window
   frame .wl.f2
   label .wl.f2.levelLabel -text "Level"
   scale .wl.f2.level -from [expr $l - $w] -to [expr $l + $w] \
     -orient horizontal -command SetLevel
   checkbutton .wl.video -text "Inverse Video" -command SetInverseVideo

   # resolutions less than 1.0
   if {$w < 10} {
      set res [expr 0.05 * $w]
      .wl.f1.window configure -resolution $res -from $res -to [expr 2.0 * $w]
      .wl.f2.level configure -resolution $res \
	-from [expr 0.0 + $l - $w] -to [expr 0.0 + $l + $w] 
   }

   .wl.f1.window set $w
   .wl.f2.level set $l
   
   frame .ex
   button .ex.exit -text "Exit" -command "exit"

   pack .ct .wl .ex -side top
   
   pack .ct.f1 .ct.f2 .ct.f3 .ct.f4 .ct.f5 -side top
   pack .ct.f1.xLabel .ct.f1.x -side left
   pack .ct.f2.yLabel .ct.f2.y -side left
   pack .ct.f3.zLabel .ct.f3.z -side left
   pack .ct.f4.oLabel .ct.f4.o -side left
   pack .ct.f5.sLabel .ct.f5.s -side left

   pack .wl.f1 .wl.f2 .wl.video -side top
   pack .wl.f1.windowLabel .wl.f1.window -side left
   pack .wl.f2.levelLabel .wl.f2.level -side left
   pack .ex.exit -side left
}


proc SetX x {
   global reslice2
   global imgWin
   set center [reslice2 GetResliceAxesOrigin]
   set y [lindex $center 1]
   set z [lindex $center 2]
   reslice2 SetResliceAxesOrigin $x $y $z
   imgWin Render
}

proc SetY y {
   global reslice1
   global imgWin
   set center [reslice1 GetResliceAxesOrigin]
   set x [lindex $center 0]
   set z [lindex $center 2]
   reslice1 SetResliceAxesOrigin $x $y $z
   imgWin Render
}

proc SetZ z {
   global reslice3 
   global imgWin
   set center [reslice3 GetResliceAxesOrigin]
   set x [lindex $center 0]
   set y [lindex $center 1]
   reslice3 SetResliceAxesOrigin $x $y $z
   imgWin Render
}

proc SetO o {
   global reslice3 
   global imgWin
   set center [reslice4 GetResliceAxesOrigin]
   set x [lindex $center 0]
   set y [lindex $center 1]
   reslice4 SetResliceAxesOrigin $x $y $o
   imgWin Render
}

proc SetSpacing spacing {
   global reslice1 reslice2 reslice3 reslice4
   global imgWin
   # don't modify the slice spacing
   set s [reslice1 GetOutputSpacing]
   set s [lindex $s 2]
   reslice1 SetOutputSpacing $spacing $spacing $s
   reslice2 SetOutputSpacing $spacing $spacing $s
   reslice3 SetOutputSpacing $spacing $spacing $s
   reslice4 SetOutputSpacing $spacing $spacing $s

   imgWin Render
}

proc SetWindow window {
   global mapper1 mapper2 mapper3 mapper4
   global imgWin
   global video
   if {$video} {
      mapper1 SetColorWindow [expr -$window]
      mapper2 SetColorWindow [expr -$window]
      mapper3 SetColorWindow [expr -$window]
      mapper4 SetColorWindow [expr -$window]
   } else {
      mapper1 SetColorWindow $window
      mapper2 SetColorWindow $window
      mapper3 SetColorWindow $window
      mapper4 SetColorWindow $window
   }
   imgWin Render
}

proc SetLevel level {
   global mapper1 mapper2 mapper3 mapper4
   global imgWin
   mapper1 SetColorLevel $level
   mapper2 SetColorLevel $level
   mapper3 SetColorLevel $level
   mapper4 SetColorLevel $level
   imgWin Render
}

proc SetInverseVideo {} {
   global mapper1 mapper2 mapper3 mapper4
   global imgWin
   global video window
   if {$video} {
      mapper1 SetColorWindow [expr -$window]
      mapper2 SetColorWindow [expr -$window]
      mapper3 SetColorWindow [expr -$window]
      mapper4 SetColorWindow [expr -$window]
   } else {
      mapper1 SetColorWindow $window
      mapper2 SetColorWindow $window
      mapper3 SetColorWindow $window
      mapper4 SetColorWindow $window
   }
   imgWin Render
}

InitializeInterface

} else {
    wm withdraw .
   imgWin Render
}

