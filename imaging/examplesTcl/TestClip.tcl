catch {load vtktcl}
# Test the object vtkImagePaint which is a region that has methods to draw
# lines and Boxs in different colors.  This extension allows the
# paint object to draw grey scale.


source vtkImageInclude.tcl

vtkImageCanvasSource2D canvas
canvas SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS 
canvas SetScalarType $VTK_UNSIGNED_CHAR
canvas SetExtent 0 711 0 711
canvas SetDrawColor 66
canvas FillBox 0 711 0 711
canvas SetDrawColor 132
canvas FillBox 32 511 100 500
canvas SetDrawColor 33
canvas FillTube 500 20 30 400 5
canvas SetDrawColor 255
canvas DrawSegment 10 20 90 510
canvas SetDrawColor 100
canvas DrawSegment 510 90 10 20

# Check Filling a triangle
canvas SetDrawColor 70
canvas FillTriangle 100 100  300 150  150 300

# Check drawing a circle
canvas SetDrawColor 84
canvas DrawCircle 350 350  200.0

# Check drawing a point
canvas SetDrawColor 250
canvas DrawPoint 350 350

# Test filling functionality
canvas SetDrawColor 48
canvas DrawCircle 450 350 80.0
canvas SetDrawColor 255
canvas FillPixel 450 350

vtkImageClip clip
clip SetInput [canvas GetOutput]
clip AutomaticOn
clip ReleaseDataFlagOff

vtkImageViewer viewer
viewer SetInput [clip GetOutput]
viewer SetColorWindow 120
viewer SetColorLevel 60
viewer Render


# make interface
source WindowLevelInterface.tcl






