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
  #sphere SetPhiResolution 3
  #sphere SetThetaResolution 4
  sphere SetPhiResolution 5
  sphere SetThetaResolution 6
  sphere SetRadius 1

sphere Update
set sphereOut [sphere GetOutput]
$sphereOut Register ""
sphere SetOutput ""

[$sphereOut GetPointData] SetNormals ""

vtkElevationFilter el
  el SetInput $sphereOut
#  el SetInput [sphere GetOutput]
  el SetLowPoint 0 0 0
  el SetHighPoint 0 0 1

vtkQuadricDecimation mesh
#  mesh SetInput $sphereOut
  mesh SetInput [el GetOutput]
  mesh SetMaximumCost 6
  mesh SetMaximumCollapsedEdges 1
#  mesh DebugOn

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