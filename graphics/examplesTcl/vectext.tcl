catch {load vtktcl}
# this is a tcl version of the Mace example
# get the interactor ui
source vtkInt.tcl
# First create the render master
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# create a sphere source and actor
#
vtkSphereSource sphere

# create the spikes using a cone source and the sphere source
#
vtkVectorText atext
set count 3
atext SetText "Welcome to VTK
An exciting new adventure 
brought to you by over 
$count monkeys at work for 
over three years."

vtkShrinkPolyData shrink
shrink SetInput [atext GetOutput]
shrink SetShrinkFactor 0.1

vtkPolyMapper spikeMapper
    spikeMapper SetInput [shrink GetOutput]
vtkActor spikeActor
    spikeActor SetMapper spikeMapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActor spikeActor
$ren1 SetBackground 0.1 0.2 0.4
$renWin SetSize 500 300
set cam1 [$ren1 GetActiveCamera]
$cam1 Zoom 2.4

for {} {$count < 27} {} {
   $renWin Render
   set count [expr $count+1]
   shrink SetShrinkFactor [expr $count / 27.0]; 
atext SetText "Welcome to VTK
An exciting new adventure 
brought to you by over 
$count monkeys at work for 
over three years."
}

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize

#$renWin SetFileName "vectext.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
