# This example shows how to use the InteractorStyleImage and add your own
# event handling.  The InteractorStyleImage is a special interactor designed
# to be used with vtkImageActor in a rendering window context. It forces the
# camera to stay perpendicular to the x-y plane. 

package require vtk
package require vtkinteraction

# Create the image
#
vtkPNGReader reader
  reader SetDataSpacing 0.8 0.8 1.5
  reader SetFileName "$VTK_DATA_ROOT/Data/fullhead15.png"

vtkImageShiftScale shiftScale
  shiftScale SetInput [reader GetOutput]
  shiftScale SetShift 0
  shiftScale SetScale 0.07
  shiftScale SetOutputScalarTypeToUnsignedChar

vtkImageActor ia
  ia SetInput [shiftScale GetOutput]

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Create an image interactor style and associate it with the 
# interactive renderer. Then assign some callbacks with the
# appropriate events. THe callbacks are implemented as Tcl procs.
vtkInteractorStyleImage interactor
  iren SetInteractorStyle interactor
  interactor AddObserver LeftButtonPressEvent {StartZoom}
  interactor AddObserver MouseMoveEvent {MouseMove}
  interactor AddObserver LeftButtonReleaseEvent {EndZoom}

# Add the actors to the renderer, set the background and size
ren1 AddActor ia
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 400 400

# render the image
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

set cam1 [ren1 GetActiveCamera]

ren1 ResetCameraClippingRange
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

### Supporting data for callbacks
vtkPoints pts
  pts SetNumberOfPoints 4
vtkCellArray lines
  lines InsertNextCell 5
  lines InsertCellPoint 0
  lines InsertCellPoint 1
  lines InsertCellPoint 2
  lines InsertCellPoint 3
  lines InsertCellPoint 0
vtkPolyData pd
  pd SetPoints pts
  pd SetLines lines
vtkPolyDataMapper2D bboxMapper
  bboxMapper SetInput pd
vtkActor2D bboxActor
  bboxActor SetMapper bboxMapper
  [bboxActor GetProperty] SetColor 1 0 0
ren1 AddProp bboxActor

### Procedures for callbacks---------------------
set X 0
set Y 0
set bboxEnabled 0

proc StartZoom {} {
  global X Y bboxEnabled

  set xy [iren GetEventPosition]
  set X [lindex $xy 0]
  set Y [lindex $xy 1]

  pts SetPoint 0 $X $Y 0
  pts SetPoint 1 $X $Y 0
  pts SetPoint 2 $X $Y 0
  pts SetPoint 3 $X $Y 0

  set bboxEnabled 1
  bboxActor VisibilityOn
}

proc MouseMove {} {
  global X Y bboxEnabled

  if { $bboxEnabled } {
    set xy [iren GetEventPosition]
    set x [lindex $xy 0]
    set y [lindex $xy 1]

    pts SetPoint 1 $x $Y 0
    pts SetPoint 2 $x $y 0
    pts SetPoint 3 $X $y 0

    renWin Render
    }
}

#Do the hard stuff: pan and dolly
proc EndZoom {} {
  global bboxEnabled

  set p1 [pts GetPoint 0]
  set p2 [pts GetPoint 2]
  set x1 [lindex $p1 0]
  set y1 [lindex $p1 1]
  set x2 [lindex $p2 0]
  set y2 [lindex $p2 1]

  ren1 SetDisplayPoint $x1 $y1 0
  ren1 DisplayToWorld
  set p1 [ren1 GetWorldPoint]
  ren1 SetDisplayPoint $x2 $y2 0
  ren1 DisplayToWorld
  set p2 [ren1 GetWorldPoint]

  set p1X [lindex $p1 0]
  set p1Y [lindex $p1 1]
  set p1Z [lindex $p1 2]

  set p2X [lindex $p2 0]
  set p2Y [lindex $p2 1]
  set p2Z [lindex $p2 2]

  set camera [ren1 GetActiveCamera]
  set focalPt [$camera GetFocalPoint]
  set focalX [lindex $focalPt 0]
  set focalY [lindex $focalPt 1]
  set focalZ [lindex $focalPt 2]
  set position [$camera GetPosition]
  set positionX [lindex $position 0]
  set positionY [lindex $position 1]
  set positionZ [lindex $position 2]
  
  set deltaX [expr $focalX - ($p1X + $p2X)/2.0]
  set deltaY [expr $focalY - ($p1Y + $p2Y)/2.0]

  #Set camera focal point to the center of the box
  $camera SetFocalPoint [expr ($p1X + $p2X)/2.0] \
          [expr ($p1Y + $p2Y)/2.0] $focalZ
  $camera SetPosition [expr $positionX - $deltaX] \
          [expr $positionY - $deltaY] $positionZ

  #Now dolly the camera to fill the box
  #This is a half-assed hack for demonstration purposes
  if { $p1X > $p2X } {
      set deltaX [expr $p1X - $p2X]
  } else {
      set deltaX [expr $p2X - $p1X]
  }
  if { $p1Y > $p2Y } {
      set deltaY [expr $p1Y - $p2Y]
  } else {
      set deltaY [expr $p2Y - $p1Y]
  }

  set winSize [renWin GetSize]
  set winX [lindex $winSize 0]
  set winY [lindex $winSize 1]

  set sx [expr $deltaX / $winX]
  set sy [expr $deltaY / $winY]

  if { $sx > $sy } {
      set dolly [expr 1.0 + 1.0/(2.0*$sx)]
  } else {
      set dolly [expr 1.0 + 1.0/(2.0*$sy)]
  }
  $camera Dolly $dolly
  ren1 ResetCameraClippingRange

  set bboxEnabled 0
  bboxActor VisibilityOff
  renWin Render
}

