catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# this is a tcl version showing diffs between flat & gouraud
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a sphere source and actor
#
vtkSphereSource sphere
sphere SetThetaResolution 30
sphere SetPhiResolution 30
vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 375 375

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize
renWin Render

set lights [ren1 GetLights]
$lights InitTraversal
set light [$lights GetNextItem]
$light SetLightTypeToSceneLight
iren LightFollowCameraOff

$cam1 Azimuth 30
$cam1 Elevation -50

set prop [sphereActor GetProperty]
$prop SetDiffuseColor 1.0 0 0
$prop SetDiffuse 0.6
$prop SetSpecularPower 5
$prop SetSpecular 0.5
renWin Render
renWin SetFileName f1.ppm
#renWin SaveImageAsPPM

$prop SetSpecular 1.0
renWin Render

#renWin SetFileName specular.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


