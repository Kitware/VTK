catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create bottle profile
#
vtkPoints points
    points InsertPoint 0 0.01 0.0 0.0
    points InsertPoint 1 1.5 0.0 0.0
    points InsertPoint 2 1.5 0.0 3.5
    points InsertPoint 3 1.25 0.0 3.75
    points InsertPoint 4 0.75 0.0 4.00
    points InsertPoint 5 0.6 0.0 4.35
    points InsertPoint 6 0.7 0.0 4.65
    points InsertPoint 7 1.0 0.0 4.75
    points InsertPoint 8 1.0 0.0 5.0
    points InsertPoint 9 0.2 0.0 5.0
vtkCellArray lines
    lines InsertNextCell 10;#number of points
    lines InsertCellPoint 0
    lines InsertCellPoint 1
    lines InsertCellPoint 2
    lines InsertCellPoint 3
    lines InsertCellPoint 4
    lines InsertCellPoint 5
    lines InsertCellPoint 6
    lines InsertCellPoint 7
    lines InsertCellPoint 8
    lines InsertCellPoint 9
vtkPolyData profile
    profile SetPoints points
    profile SetLines lines

# extrude profile to make bottle
#
vtkRotationalExtrusionFilter extrude
    extrude SetInput profile
    extrude SetResolution 60
    
#vtkTriangleFilter tf
#tf SetInput [extrude GetOutput]

#vtkPolyDataNormals pn
#pn SetInput [tf GetOutput]

vtkPolyDataMapper map
    map SetInput [extrude GetOutput]

vtkActor bottle
    bottle SetMapper map
    [bottle GetProperty] SetColor 0.3800 0.7000 0.1600
#[bottle GetProperty] BackfaceCullingOff
#[bottle GetProperty] FrontfaceCullingOn

# Add the actors to the renderer, set the background and size
#
ren1 AddActor bottle
ren1 SetBackground 1 1 1

renWin SetSize 500 500
renWin Render

set cam1 [ren1 GetActiveCamera]
#$cam1 SetClippingRange 3.95297 50
#$cam1 SetFocalPoint 9.71821 0.458166 29.3999
#$cam1 SetPosition 2.7439 -37.3196 38.7167
#$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName bottle.tcl.ppm
#renWin SaveImageAsPPM


