catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

## Test the rotational extrusion filter and tube generator. Sweep a spiral
## curve to generate a tube.

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# create bottle profile
#
# draw the arrows
vtkPoints points
    points InsertNextPoint 1 0 0
    points InsertNextPoint 0.707 0.707 1
    points InsertNextPoint 0 1 2
    points InsertNextPoint -0.707 0.707 3
    points InsertNextPoint -1 0 4
    points InsertNextPoint -0.707 -0.707 5
    points InsertNextPoint 0 -1 6
    points InsertNextPoint 0.707 -0.707 7
vtkCellArray lines
    lines InsertNextCell 8
    lines InsertCellPoint 0
    lines InsertCellPoint 1
    lines InsertCellPoint 2
    lines InsertCellPoint 3
    lines InsertCellPoint 4
    lines InsertCellPoint 5
    lines InsertCellPoint 6
    lines InsertCellPoint 7
vtkPolyData profile
    profile SetPoints points
    profile SetLines lines

#create the tunnel
vtkRotationalExtrusionFilter extrude
    extrude SetInput profile
    extrude SetAngle 360
    extrude SetResolution 80
vtkCleanPolyData clean;#get rid of seam
    clean SetInput [extrude GetOutput]
    clean SetTolerance 0.001
vtkPolyDataNormals normals
    normals SetInput [clean GetOutput]
    normals SetFeatureAngle 90
vtkPolyDataMapper map
    map SetInput [normals GetOutput]
vtkActor sweep
    sweep SetMapper map
    [sweep GetProperty] SetColor 0.3800 0.7000 0.1600

#create the seam
vtkTubeFilter tuber
    tuber SetInput profile
    tuber SetNumberOfSides 6
    tuber SetRadius 0.1
vtkPolyDataMapper tubeMapper
    tubeMapper SetInput [tuber GetOutput]
vtkActor seam
    seam SetMapper tubeMapper
    [seam GetProperty] SetColor 1.0000 0.3882 0.2784

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor sweep
ren1 AddActor seam
ren1 SetBackground 1 1 1
ren1 TwoSidedLightingOn

vtkCamera acam
    acam SetClippingRange 1.38669 69.3345
    acam SetFocalPoint -0.0368406 0.191581 3.37003
    acam SetPosition 13.6548 2.10315 2.28369
    acam SetViewAngle 30
    acam SetViewUp 0.157669 -0.801427 0.576936

ren1 SetActiveCamera acam

renWin SetSize 400 400
renWin Render

iren SetUserMethod {wm deiconify .vtkInteract}
renWin SetFileName "sweptCurve.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

