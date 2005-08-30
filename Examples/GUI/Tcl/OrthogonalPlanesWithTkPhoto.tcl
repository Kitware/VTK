
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

vtkImageCast cast
cast SetInputConnection [reader GetOutputPort]
cast SetOutputScalarType [[reader GetOutput] GetScalarType]
cast ClampOverflowOn

# Make the image a little bigger
vtkImageResample resample
resample SetInputConnection [cast GetOutputPort]
resample SetAxisMagnificationFactor 0 2
resample SetAxisMagnificationFactor 1 2
resample SetAxisMagnificationFactor 2 1

set range [[reader GetOutput] GetScalarRange]
set l [lindex $range 0]
set h [lindex $range 1]

wm withdraw .
toplevel .c
wm title .c "Tcl Version of vtkImageDataToTkPhoto"
wm protocol .c WM_DELETE_WINDOW ::vtk::cb_exit

# Create the three orthogonal views
set mode 0
set m [menu .c.mm -tearoff 0]
$m add radiobutton -label "unsigned char" -value -1 -variable mode -command CastToUnsignedChar
$m add radiobutton -label "unsigned short" -value 0 -variable mode -command CastToUnsignedShort
$m add radiobutton -label "unsigned int" -value 1 -variable mode -command CastToUnsignedInt
$m add radiobutton -label "float" -value 2 -variable mode -command CastToFloat

set tphoto [image create photo]
set cphoto [image create photo]
set sphoto [image create photo]
grid [label .c.t -image $tphoto] -row 0 -column 0
bind .c.t <Button1-Motion> "SetPosition transverse %W %x %y"
bind .c.t <Button-3> "$m post %X %Y"
grid [label .c.c -image $cphoto] -row 1 -column 0
bind .c.c <Button1-Motion> "SetPosition coronal %W %x %y"
bind .c.c <Button-3> "$m post %X %Y"
grid [label .c.s -image $sphoto] -row 0 -column 1
bind .c.s <Button1-Motion> "SetPosition sagittal %W %x %y"
bind .c.s <Button-3> "$m post %X %Y"

grid [scale .c.w -label Window -orient horizontal -from 1 -to [expr ($h - $l) / 2] -command SetWindow ] -row 2 -columnspan 2 -sticky ew
grid [scale .c.l -label Level -orient horizontal -from $l -to $h -command SetWindow ] -row 3 -columnspan 2 -sticky ew
grid [label .c.text -textvariable Label -bd 2 -relief raised] -row 4 -columnspan 2 -sticky ew
set Label "Use the right mouse button to change data type"
.c.w set 1370
.c.l set 1268
reader Update

set d [[reader GetOutput] GetDimensions]
set Position(x) [expr int ( [lindex $d 0] / 2.0 ) ]
set Position(y) [expr int ( [lindex $d 1] / 2.0 ) ]
set Position(z) [expr int ( [lindex $d 2] / 2.0 ) ]
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
  SetImages
}

proc SetImages {} {
  global Position tphoto sphoto cphoto
  set Window [.c.w get]
  set Level [.c.l get]
  vtkImageDataToTkPhoto [resample GetOutput] $tphoto $Position(z) transverse $Window $Level
  vtkImageDataToTkPhoto [resample GetOutput] $sphoto $Position(x) sagittal $Window $Level
  vtkImageDataToTkPhoto [resample GetOutput] $cphoto $Position(y) coronal $Window $Level
}

proc CastToUnsignedChar {} {
  cast SetOutputScalarTypeToUnsignedChar
  SetImages
}
proc CastToUnsignedShort {} {
  cast SetOutputScalarTypeToUnsignedShort
  SetImages
}
proc CastToUnsignedInt {} {
  cast SetOutputScalarTypeToUnsignedInt
  SetImages
}
proc CastToFloat {} {
  cast SetOutputScalarTypeToFloat
  SetImages
}

# Prime the pump
SetImages
#///////////////////////////////////////////////////

