catch {load vtktcl} 
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# This is an example of how to define your own interaction methods
# in Python or Tcl

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren
vtkRenderWindow renWin
    renWin AddRenderer ren 
vtkInteractorStyleUser style
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin
    iren SetInteractorStyle style 

# create a plane source and actor
vtkPlaneSource plane
vtkPolyDataMapper planeMapper
    planeMapper SetInput [plane GetOutput]
vtkActor planeActor
    planeActor SetMapper planeMapper

# Add the actors to the renderer, set the background and size
ren AddActor planeActor 
ren SetBackground 0.1 0.2 0.4 

# push plane along its normal
proc PushPlane {} {
    set x [lindex [style GetLastPos] 0]
    set oldx [lindex [style GetOldPos] 0]
    if {$x != $oldx} {
	plane Push [expr 0.005 * ($x - $oldx)]
        iren Render
    }
}

# if user clicked actor, start push interaction
proc StartPushPlane {} {
    style StartUserInteraction
}

# end push interaction
proc EndPushPlane {} {
    style EndUserInteraction
}

# set the methods for pushing a plane
style SetMiddleButtonPressMethod StartPushPlane 
style SetMiddleButtonReleaseMethod EndPushPlane 
style SetUserInteractionMethod PushPlane 

# render the image
iren Initialize
set cam1 [ren GetActiveCamera]
$cam1 Elevation -30 
$cam1 Roll -20 
renWin Render

#renWin SetFileName "CustomInteraction.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
