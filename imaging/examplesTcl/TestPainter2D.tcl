# Use the painter to draw using colors.
# This is not a pipeline object.  It will support pipeline objects.
# Please do not use this object directly.

catch {load vtktcl}

source vtkImageInclude.tcl

vtkImagePainter2D canvas
canvas SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS 
canvas SetScalarType $VTK_UNSIGNED_CHAR
canvas SetExtent 0 511 0 511 0 2
canvas SetDrawColor 100 100 0
canvas FillBox 0 511 0 511
canvas SetDrawColor 200 0 200
canvas FillBox 32 511 100 500
canvas SetDrawColor 100 0 0
canvas FillTube 500 20 30 400 5
canvas SetDrawColor 255 255 255
canvas DrawSegment 10 20 90 510
canvas SetDrawColor 200 50 50
canvas DrawSegment 510 90 10 20

# Check segment clipping
canvas SetDrawColor 0 200 0
canvas DrawSegment -10 30 30 -10
canvas DrawSegment -10 481 30 521
canvas DrawSegment 481 -10 521 30
canvas DrawSegment 481 521 521 481

# Check Filling a triangle
canvas SetDrawColor 20 200 200
canvas FillTriangle 100 100  300 150  150 300

# Check drawing a circle
canvas SetDrawColor 250 250 10
canvas DrawCircle 350 350  200.0

# Check drawing a point
canvas SetDrawColor 250 250 250
canvas DrawPoint 350 350


# Test filling functionality
canvas SetDrawColor 55 0 0
canvas DrawCircle 450 350 80.0
canvas SetDrawColor 100 255 100
canvas FillPixel 450 350




vtkImageViewer viewer
viewer SetInput [canvas GetOutput]
viewer SetColorWindow 256
viewer SetColorLevel 128
viewer ColorFlagOn


# make interface
source WindowLevelInterface.tcl





