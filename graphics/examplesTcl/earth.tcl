catch {load vtktcl}
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

vtkTexturedSphereSource tss
tss SetThetaResolution 18
tss SetPhiResolution 9
vtkPolyMapper   earthMapper
earthMapper SetInput [tss GetOutput]
vtkActor earthActor
earthActor SetMapper earthMapper

# load in the texture map
#
vtkTexture atext
vtkPNMReader pnmReader
pnmReader SetFileName "../../../data/earth.ppm"
atext SetInput [pnmReader GetOutput]
atext InterpolateOn
earthActor SetTexture atext

# create a earth source and actor
#
vtkEarthSource es
es SetRadius 0.501
es SetOnRatio 2
vtkPolyMapper   earth2Mapper
earth2Mapper SetInput [es GetOutput]
vtkActor earth2Actor
earth2Actor SetMapper earth2Mapper

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors earthActor
$ren1 AddActors earth2Actor
$ren1 SetBackground 0 0 0.1
$renWin SetSize 300 300

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [$ren1 GetActiveCamera]
$cam1 Zoom 1.4
$iren Initialize
#$renWin SetFileName "earth.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


