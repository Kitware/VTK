catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Demonstrate how to extract polygonal cells with an implicit function
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# create a sphere source and actor
#
vtkSphereSource sphere
  sphere SetThetaResolution 8
  sphere SetPhiResolution 16
  sphere SetRadius 1.5

# Extraction stuff
vtkTransform t
    t RotateX 90
vtkCylinder cylfunc
    cylfunc SetRadius 0.5
    cylfunc SetTransform t
vtkExtractPolyDataGeometry extract
    extract SetInput [sphere GetOutput]
    extract SetImplicitFunction cylfunc
    extract ExtractBoundaryCellsOn

vtkPolyDataMapper  sphereMapper
    sphereMapper SetInput [extract GetOutput]
    sphereMapper GlobalImmediateModeRenderingOn

vtkActor sphereActor
    sphereActor SetMapper sphereMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin SetWindowName "vtk - extractPolyData"
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sphereActor

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


