catch {load vtktcl}
# First create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren   [$renWin MakeRenderWindowInteractor]

# create a plane source and actor
vtkPlaneSource plane
vtkPolyMapper  planeMapper
planeMapper SetInput [plane GetOutput]
vtkActor planeActor
planeActor SetMapper planeMapper

# load in the texture map
#
vtkTexture atext
vtkPNMReader pnmReader
pnmReader SetFileName "../../../data/masonry.ppm"
atext SetInput [pnmReader GetOutput]
atext InterpolateOn
planeActor SetTexture atext

# Add the actors to the renderer, set the background and size
$ren1 AddActors planeActor
$ren1 SetBackground 0.1 0.2 0.4
$renWin SetSize 500 500

# render the image
$iren Initialize
set cam1 [$ren1 GetActiveCamera]
$cam1 Elevation -30
$cam1 Roll -20
$renWin Render

#$renWin SetFileName "TPlane.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .





