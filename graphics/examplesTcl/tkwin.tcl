catch {load vtktcl}
catch {load vtktcl}
wm withdraw .
toplevel .top -width 300 -height 300 -visual {truecolor 24} 
wm title .top {VTK and TK}
wm positionfrom .top program
wm geometry .top +50+50
 
source vtkInt.tcl
 
# First create the render master
#
vtkRenderMaster rm
 
# Now create the RenderWindow, Renderer and both Actors
#
set renWin1  [rm MakeRenderWindow]
set ren1     [$renWin1 MakeRenderer]
set iren1    [$renWin1 MakeRenderWindowInteractor]
 
# create a sphere source and actor
#
vtkSphereSource sphere
vtkPolyMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

 
# create the spikes using a cone source and the sphere source
#
vtkConeSource cone
vtkGlyph3D glyph
    glyph SetInput [sphere GetOutput]
    glyph SetSource [cone GetOutput]
    glyph UseNormal
    glyph ScaleByVector
    glyph SetScaleFactor 0.25
vtkPolyMapper spikeMapper
    spikeMapper SetInput [glyph GetOutput]
vtkLODActor spikeActor
    spikeActor SetMapper spikeMapper
 
# Add the actors to the renderer, set the background and size
#
$ren1 AddActors sphereActor
$ren1 AddActors spikeActor
$ren1 SetBackground 0.1 0.2 0.4
 
# render the image
#
$iren1 SetUserMethod {wm deiconify .vtkInteract}
set cam1 [$ren1 GetActiveCamera]
$cam1 Zoom 1.4

label .top.label -relief ridge -borderwidth 6 -text "Both Tk and VTK"
canvas .top.renwin -width 300 -height 300 -highlightthickness 0
pack .top.label .top.renwin -expand 1 -fill both

$renWin1 SetTkWindow .top.renwin
$iren1 Initialize
