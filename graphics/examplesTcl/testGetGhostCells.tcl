catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# demonstrate use of point labeling and the selection window

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkSphereSource sphere1
	sphere1 SetStartTheta 0
	sphere1 SetEndTheta 120

vtkSphereSource sphere2
	sphere2 SetStartTheta 120
	sphere2 SetEndTheta 240

vtkSphereSource sphere3
	sphere3 SetStartTheta 240
	sphere3 SetEndTheta 360

vtkGetPolyDataGhostCells ghostCells
	ghostCells AddInput [sphere1 GetOutput]
	ghostCells AddInput [sphere2 GetOutput]
	ghostCells AddInput [sphere3 GetOutput]
	
#vtkGeometryFilter geometry
#	geometry SetInput [ghostCells GetOutput]
	
vtkPolyDataMapper mapper
	mapper SetInput [ghostCells GetOutput]
	mapper SetGhostLevel 3
	
vtkActor actor
	actor SetMapper mapper

[ren1 GetActiveCamera] SetPosition 1 -2 2
[ren1 GetActiveCamera] SetFocalPoint 0 0 0
ren1 AddActor actor

ren1 SetBackground 1 1 1
renWin SetSize 500 500
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
