# Test the object vtkImagePaint which is a region that has methods to draw
# lines and Boxs in different colors.  This extension allows the
# paint object to draw grey scale.


catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source vtkImageInclude.tcl

vtkImageCanvasSource2D imageCanvas
imageCanvas SetNumberOfScalarComponents 3
imageCanvas SetScalarType $VTK_UNSIGNED_CHAR
imageCanvas SetExtent 0 511 0 511 0 0
imageCanvas SetDrawColor 100 100 0
imageCanvas FillBox 0 511 0 511
imageCanvas SetDrawColor 200 0 200
imageCanvas FillBox 32 511 100 500
imageCanvas SetDrawColor 100 0 0
imageCanvas FillTube 550 20 30 400 5
imageCanvas SetDrawColor 255 255 255
imageCanvas DrawSegment 10 20 90 510
imageCanvas SetDrawColor 200 50 50
imageCanvas DrawSegment 510 90 10 20

# Check segment clipping
imageCanvas SetDrawColor 0 200 0
imageCanvas DrawSegment -10 30 30 -10
imageCanvas DrawSegment -10 481 30 521
imageCanvas DrawSegment 481 -10 521 30
imageCanvas DrawSegment 481 521 521 481

# Check Filling a triangle
imageCanvas SetDrawColor 20 200 200
imageCanvas FillTriangle -100 100  190 150  40 300

# Check drawing a circle
imageCanvas SetDrawColor 250 250 10
imageCanvas DrawCircle 350 350  200.0

# Check drawing a point
imageCanvas SetDrawColor 250 250 250
imageCanvas DrawPoint 350 350
imageCanvas DrawPoint 350 550


# Test filling functionality
imageCanvas SetDrawColor 55 0 0
imageCanvas DrawCircle 450 350 80.0
imageCanvas SetDrawColor 100 255 100
imageCanvas FillPixel 450 350

vtkImageClip clip
clip SetInput [imageCanvas GetOutput]
clip SetOutputWholeExtent 0 255 0 255 0 0
clip ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [clip GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 127.5


# make interface
source WindowLevelInterface.tcl






