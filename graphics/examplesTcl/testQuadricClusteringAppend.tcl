catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Generate implicit model of a sphere
#
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# pipeline stuff
#

vtkSphereSource sphere
  sphere SetPhiResolution 150
  sphere SetThetaResolution 75
  sphere SetEndTheta 180
  sphere Update

vtkSphereSource sphere2
  sphere2 SetPhiResolution 150
  sphere2 SetThetaResolution 75
  sphere2 SetStartTheta 180
  sphere2 Update

vtkQuadricClustering mesh
#  mesh SetInput [sphere GetOutput]
#  mesh AddInput [sphere2 GetOutput]
  mesh SetNumberOfXDivisions 30
  mesh SetNumberOfYDivisions 30
  mesh SetNumberOfZDivisions 30
  mesh StartAppend -.5 .5 -.5 .5 -.5 .5
  mesh Append [sphere GetOutput]
  mesh Append [sphere2 GetOutput]
  mesh EndAppend


vtkPolyDataMapper mapper
  mapper SetInput [mesh GetOutput]
vtkActor actor
  actor SetMapper mapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 SetBackground 1 1 1
renWin SetSize 500 500
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw .