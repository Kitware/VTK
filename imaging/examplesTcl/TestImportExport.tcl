catch {load vtktcl}
# Test the object vtkImagePaint which is a region that has methods to draw
# lines and Boxs in different colors.

source vtkImageInclude.tcl

# Make some test image
vtkImagePaint canvas
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
canvas SetDrawColor 20 200 200
canvas FillTriangle 100 100  300 150  150 300
canvas SetDrawColor 250 250 10
canvas DrawCircle 350 350  200.0
canvas SetDrawColor 250 250 250
canvas DrawPoint 350 350
canvas SetDrawColor 55 0 0
canvas DrawCircle 450 350 80.0
canvas SetDrawColor 100 255 100
canvas FillPixel 450 350

vtkImageExport export
#export DebugOn
export SetInput [canvas GetOutput]
export SetAxes 4 0 1
export SetScalarType $VTK_UNSIGNED_CHAR
export SetExtent 0 2 0 511 0 511

vtkImageImport import
#import DebugOn
import SetAxes 4 0 1
import SetScalarType $VTK_UNSIGNED_CHAR
import SetExtent 0 2 0 511 0 511
import TestExport export

vtkImageViewer viewer
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS $VTK_IMAGE_COMPONENT_AXIS
viewer SetInput [import GetOutput]
viewer SetColorWindow 150
viewer SetColorLevel 13
viewer ColorFlagOn
viewer Render


#make interface
#

frame .wl
frame .wl.f1
label .wl.f1.windowLabel -text Window
scale .wl.f1.window -from 1 -to 512 -orient horizontal -command SetWindow
frame .wl.f2
label .wl.f2.levelLabel -text Level
scale .wl.f2.level -from 1 -to 256 -orient horizontal -command SetLevel
checkbutton .wl.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo


.wl.f1.window set 256
.wl.f2.level set 128


pack .wl -side left
pack .wl.f1 .wl.f2 .wl.video -side top
pack .wl.f1.windowLabel .wl.f1.window -side left
pack .wl.f2.levelLabel .wl.f2.level -side left



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

proc SetInverseVideo {} {
   global viewer
   if { $inverseVideo == 0 } {
      viewer SetWindow -255
   } else {
      viewer SetWindow 255
   }		
   viewer Render
}





#$renWin Render
#wm withdraw .








