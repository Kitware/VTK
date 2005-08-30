#
# This example creates a polygonal model of a cone, and then renders it to
# the screen. It also defines an interaction style by creating a set
# of Command/Observers. (Note: it is far more efficient to create new
# styles by subclassing vtkInteractorStyle. This is just an illustrative
# example.) If you really want trackball behavior, look at the 
# vtkInteractorStyleTrackballCamera class.
#

#
# First we include the VTK Tcl packages which will make available 
# all of the VTK commands to Tcl.
#
package require vtk

# 
# Next we create an instance of vtkConeSource and set some of its
# properties. The instance of vtkConeSource "cone" is part of a visualization
# pipeline (it is a source process object); it produces data (output type is
# vtkPolyData) which other filters may process.
#
vtkConeSource cone
cone SetHeight 3.0
cone SetRadius 1.0
cone SetResolution 10

# 
# In this example we terminate the pipeline with a mapper process object.
# (Intermediate filters such as vtkShrinkPolyData could be inserted in
# between the source and the mapper.)  We create an instance of
# vtkPolyDataMapper to map the polygonal data into graphics primitives. We
# connect the output of the cone souece to the input of this mapper.
#
vtkPolyDataMapper coneMapper
coneMapper SetInputConnection [cone GetOutputPort]

# 
# Create an actor to represent the cone. The actor orchestrates rendering of
# the mapper's graphics primitives. An actor also refers to properties via a
# vtkProperty instance, and includes an internal transformation matrix. We
# set this actor's mapper to be coneMapper which we created above.
#
vtkActor coneActor
coneActor SetMapper coneMapper

#
# Create the Renderer and assign actors to it. A renderer is like a
# viewport. It is part or all of a window on the screen and it is responsible
# for drawing the actors it has.  We also set the background color here.
#
vtkRenderer ren1 
ren1 AddActor coneActor
ren1 SetBackground 0.1 0.2 0.4

#
# Finally we create the render window which will show up on the screen
# We put our renderer into the render window using AddRenderer. We also
# set the size to be 300 pixels by 300.
#
vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 300 300

#
# Define custom interaction.
#
vtkRenderWindowInteractor iren
iren SetInteractorStyle ""
iren SetRenderWindow renWin

#
# Add the observers to watch for particular events. These invoke
# Tcl procedures.
#
set Rotating 0
set Panning 0
set Zooming 0
iren AddObserver LeftButtonPressEvent {global Rotating; set Rotating 1}
iren AddObserver LeftButtonReleaseEvent {global Rotating; set Rotating 0}
iren AddObserver MiddleButtonPressEvent {global Panning; set Panning 1}
iren AddObserver MiddleButtonReleaseEvent {global Panning; set Panning 0}
iren AddObserver RightButtonPressEvent {global Zooming; set Zooming 1}
iren AddObserver RightButtonReleaseEvent {global Zooming; set Zooming 0}
iren AddObserver MouseMoveEvent MouseMove
iren AddObserver KeyPressEvent Keypress

# General high-level logic
#
proc MouseMove {} {
  global Rotating Panning Zooming

  set lastXYpos [iren GetLastEventPosition]
  set lastX [lindex $lastXYpos 0]
  set lastY [lindex $lastXYpos 1]

  set xypos [iren GetEventPosition]
  set x [lindex $xypos 0]
  set y [lindex $xypos 1]

  set center [renWin GetSize]
  set centerX [expr [lindex $center 0] / 2.0]
  set centerY [expr [lindex $center 1] / 2.0]

  if { $Rotating } {
      Rotate ren1 [ren1 GetActiveCamera] $x $y $lastX $lastY $centerX $centerY
  } elseif { $Panning } {
      Pan ren1 [ren1 GetActiveCamera] $x $y $lastX $lastY $centerX $centerY
  } elseif { $Zooming } {
      Dolly ren1 [ren1 GetActiveCamera] $x $y $lastX $lastY $centerX $centerY
  }
}

proc Keypress {} {
  set key [iren GetKeySym]
    if { $key == "e" } {
        vtkCommand DeleteAllObjects
        exit
    } elseif { $key == "w" } {
        Wireframe
    } elseif { $key == "s" } {
        Surface
    }
}

#
# Routines that translate the events into camera motions.
#
# This one is associated with the left mouse button. It translates x and y
# relative motions into camera azimuth and elevation commands.
#
proc Rotate {renderer camera x y lastX lastY centerX centerY} {

    $camera Azimuth [expr ($lastX - $x)]
    $camera Elevation [expr ($lastY - $y)]
    $camera OrthogonalizeViewUp

    renWin Render
}

# Pan translates x-y motion into translation of the focal point and position.
#
proc Pan {renderer camera x y lastX lastY centerX centerY} {

    set FPoint [$camera GetFocalPoint]
        set FPoint0 [lindex $FPoint 0]
        set FPoint1 [lindex $FPoint 1]
        set FPoint2 [lindex $FPoint 2]

    set PPoint [$camera GetPosition]
        set PPoint0 [lindex $PPoint 0]
        set PPoint1 [lindex $PPoint 1]
        set PPoint2 [lindex $PPoint 2]

    $renderer SetWorldPoint $FPoint0 $FPoint1 $FPoint2 1.0
    $renderer WorldToDisplay
    set DPoint [$renderer GetDisplayPoint]
    set focalDepth [lindex $DPoint 2]

    set APoint0 [expr $centerX + ($x - $lastX)]
    set APoint1 [expr $centerY + ($y - $lastY)]

    $renderer SetDisplayPoint $APoint0 $APoint1 $focalDepth
    $renderer DisplayToWorld
    set RPoint [$renderer GetWorldPoint]
        set RPoint0 [lindex $RPoint 0]
        set RPoint1 [lindex $RPoint 1]
        set RPoint2 [lindex $RPoint 2]
        set RPoint3 [lindex $RPoint 3]
    if { $RPoint3 != 0.0 } {
        set RPoint0 [expr $RPoint0 / $RPoint3]
        set RPoint1 [expr $RPoint1 / $RPoint3]
        set RPoint2 [expr $RPoint2 / $RPoint3]
    }

    $camera SetFocalPoint \
      [expr ($FPoint0 - $RPoint0)/2.0 + $FPoint0] \
      [expr ($FPoint1 - $RPoint1)/2.0 + $FPoint1] \
      [expr ($FPoint2 - $RPoint2)/2.0 + $FPoint2]

    $camera SetPosition \
      [expr ($FPoint0 - $RPoint0)/2.0 + $PPoint0] \
      [expr ($FPoint1 - $RPoint1)/2.0 + $PPoint1] \
      [expr ($FPoint2 - $RPoint2)/2.0 + $PPoint2]

    renWin Render
}

# Dolly converts y-motion into a camera dolly commands.

proc Dolly {renderer camera x y lastX lastY centerX centerY} {
    set dollyFactor [expr pow(1.02,(0.5*($y - $lastY)))]

    if {[$camera GetParallelProjection]} {
	set parallelScale [expr [$camera GetParallelScale] * $dollyFactor];
	$camera SetParallelScale $parallelScale;
    } else {
	$camera Dolly $dollyFactor
	$renderer ResetCameraClippingRange
    }

    renWin Render
}

# Wireframe sets the representation of all actors to wireframe.
#
proc Wireframe {} {
    set actors [ren1 GetActors]

    $actors InitTraversal
    set actor [$actors GetNextItem]
    while { $actor != "" } {
        [$actor GetProperty] SetRepresentationToWireframe
        set actor [$actors GetNextItem]
    }

    renWin Render
}

# Surface sets the representation of all actors to surface.
#
proc Surface {} {
    set actors [ren1 GetActors]

    $actors InitTraversal
    set actor [$actors GetNextItem]
    while { $actor != "" } {
        [$actor GetProperty] SetRepresentationToSurface
        set actor [$actors GetNextItem]
    }

    renWin Render
}

iren Initialize
wm withdraw .



