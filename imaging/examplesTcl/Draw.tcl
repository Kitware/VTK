# Use the CanvasSource to draw in gray scale.
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source vtkImageInclude.tcl

vtkImageCanvasSource2D imageCanvas
imageCanvas SetScalarType $VTK_UNSIGNED_CHAR
imageCanvas SetExtent 0 511 0 511 0 0
imageCanvas SetDrawColor 66
imageCanvas FillBox 0 511 0 511
imageCanvas SetDrawColor 132
imageCanvas FillBox 32 511 100 500
imageCanvas SetDrawColor 33
imageCanvas FillTube 500 20 30 400 5
imageCanvas SetDrawColor 255
imageCanvas DrawSegment 10 20 90 510
imageCanvas SetDrawColor 100
imageCanvas DrawSegment 510 90 10 20

# Check segment clipping
imageCanvas SetDrawColor 66
imageCanvas DrawSegment -10 30 30 -10
imageCanvas DrawSegment -10 481 30 521
imageCanvas DrawSegment 481 -10 521 30
imageCanvas DrawSegment 481 521 521 481

# Check Filling a triangle
imageCanvas SetDrawColor 70
imageCanvas FillTriangle 100 100  300 150  150 300

# Check drawing a circle
imageCanvas SetDrawColor 84
imageCanvas DrawCircle 350 350  200.0

# Check drawing a point
imageCanvas SetDrawColor 250
imageCanvas DrawPoint 350 350

# Test filling functionality
imageCanvas SetDrawColor 48
imageCanvas DrawCircle 450 350 80.0
imageCanvas SetDrawColor 255
imageCanvas FillPixel 450 350


vtkImageViewer viewer
viewer SetInput [imageCanvas GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5


# make interface
source WindowLevelInterface.tcl






