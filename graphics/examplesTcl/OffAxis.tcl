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
set ren2   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# create a sphere source and actor
#
vtkSphereSource sphere
vtkPolyMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkActor sphereActor
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
vtkActor spikeActor
    spikeActor SetMapper spikeMapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors sphereActor
$ren1 AddActors spikeActor
$ren1 SetBackground 0.1 0.2 0.4
$ren1 SetViewport 0 0 0.5 1
$ren2 AddActors sphereActor
$ren2 AddActors spikeActor
$ren2 SetBackground 0.1 0.4 0.2
$ren2 SetViewport 0.5 0 1 1
$renWin SetSize 500 500

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
$iren Initialize
set cam1 [$ren1 GetActiveCamera]
set cam2 [$ren2 GetActiveCamera]
$cam1 SetWindowCenter -1.01 0
$cam2 SetWindowCenter 1.01 0

$renWin Render
#$renWin SetFileName OffAxis.tcl.ppm
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


