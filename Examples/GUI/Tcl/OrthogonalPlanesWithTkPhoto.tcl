
#////////////////////////////////////////////////
package require vtk
package require vtkinteraction

# Image pipeline

vtkVolume16Reader reader
  reader SetDataDimensions 64 64  
  reader SetDataByteOrderToLittleEndian  
  reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"   
  reader SetImageRange 1 93  
  reader SetDataSpacing 3.2 3.2 1.5  
  reader Update 

# Make the image a little bigger
vtkImageResample resample
resample SetInput [reader GetOutput]
resample SetAxisMagnificationFactor 0 2
resample SetAxisMagnificationFactor 1 2
resample SetAxisMagnificationFactor 2 1

vtkImageShiftScale cast
cast SetInput [resample GetOutput]
cast SetOutputScalarTypeToUnsignedChar
cast ClampOverflowOn
cast Update

set range [[reader GetOutput] GetScalarRange]
set l [lindex $range 0]
set h [lindex $range 1]

wm withdraw .
toplevel .c
wm title .c "Tcl Version of vtkImageDataToTkPhoto"
wm protocol .c WM_DELETE_WINDOW ::vtk::cb_exit

# Create the three orthogonal views

set tphoto [image create photo]
set cphoto [image create photo]
set sphoto [image create photo]
grid [label .c.t -image $tphoto] -row 0 -column 0
bind .c.t <Button1-Motion> "SetPosition transverse %W %x %y"
grid [label .c.c -image $cphoto] -row 1 -column 0
bind .c.c <Button1-Motion> "SetPosition coronal %W %x %y"
grid [label .c.s -image $sphoto] -row 0 -column 1
bind .c.s <Button1-Motion> "SetPosition sagittal %W %x %y"

grid [scale .c.w -label Window -orient horizontal -from 1 -to [expr ($h - $l) / 2] -command SetWindow ] -row 2 -columnspan 2 -sticky ew
grid [scale .c.l -label Level -orient horizontal -from $l -to $h -command SetWindow ] -row 3 -columnspan 2 -sticky ew
grid [label .c.text -textvariable Label -bd 2 -relief raised] -row 4 -columnspan 2 -sticky ew
.c.w set 1370
.c.l set 1268
set Position(x) 0
set Position(y) 0
set Position(z) 0
# Scale = 255 / window
# Shift = Window / 2 - level

proc SetPosition { orientation widget x y } {
  global Label Position
  set i [$widget cget -image]
  set w [image width $i]
  set h [image height $i]
  switch $orientation {
    transverse { set Position(x) $x; set Position(y) [expr $h - $y - 1] }
    coronal { set Position(x) $x; set Position(z) $y }
    sagittal { set Position(y) [expr $w - $x - 1]; set Position(z) $y }
  }
  set Label "$orientation Position: $Position(x), $Position(y), $Position(z)"
  SetImages
}

proc SetWindow { foo } {
  global cast photo
  set Window [.c.w get]
  set Level [.c.l get]
  cast SetScale [expr 255.0 / $Window]
  cast SetShift [expr $Window / 2.0 - $Level]
  SetImages
}

proc SetImages {} {
  global Position tphoto sphoto cphoto
  vtkImageDataToTkPhoto [cast GetOutput] $tphoto $Position(z) transverse
  vtkImageDataToTkPhoto [cast GetOutput] $sphoto $Position(x) sagittal
  vtkImageDataToTkPhoto [cast GetOutput] $cphoto $Position(y) coronal
}
# Prime the pump
SetImages
#///////////////////////////////////////////////////

