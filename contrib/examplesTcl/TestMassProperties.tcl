catch {load vtktcl}
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSphereSource sphere
   sphere SetRadius 2.5

vtkCubeSource cube
   cube SetXLength 5
   cube SetYLength 5
   cube SetZLength 5

vtkPolyDataMapper   sphereMapper
    sphereMapper SetInput [sphere GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn

vtkLODActor sphereActor
    sphereActor SetMapper sphereMapper
#Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300
# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4
iren Initialize
proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}


# create a sphere source
#

set filename "/home/alyassin2/tks/ve/VESA/models/trachea.vtk"
vtkPolyDataReader reader;
    reader SetFileName $filename

vtkTriangleFilter c;
c SetInput [sphere GetOutput]


vtkMassProperties b;
b SetInput  [c GetOutput]
b Update;

# prevent the tk window from showing up then start the event loop
wm withdraw .


