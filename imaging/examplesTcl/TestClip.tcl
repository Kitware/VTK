catch {load vtktcl}
# Test the object vtkImagePaint which is a region that has methods to draw
# lines and Boxs in different colors.  This extension allows the
# paint object to draw grey scale.


source vtkImageInclude.tcl

vtkImagePaint canvas
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

vtkImageXViewer viewer
viewer SetAxes $VTK_IMAGE_X_AXIS $VTK_IMAGE_Y_AXIS
viewer SetInput [clip GetOutput]
viewer SetColorWindow 120
viewer SetColorLevel 60
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


.wl.f1.window set 120
.wl.f2.level set 60


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


puts "Done"


#$renWin Render
#wm withdraw .








