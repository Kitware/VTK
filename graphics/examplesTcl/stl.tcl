catch {load vtktcl}
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

vtkSTLReader sr
    sr SetFileName ../../data/42400-IDGH.stl

vtkPolyMapper   stlMapper
    stlMapper SetInput [sr GetOutput]
vtkLODActor stlActor
    stlActor SetMapper stlMapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors stlActor
$ren1 SetBackground 0.1 0.2 0.4
$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [$ren1 GetActiveCamera]
$cam1 Zoom 1.4
$iren Initialize
#$renWin SetFileName "stl.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
