catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create room profile
#
vtkFloatPoints points
    points InsertPoint 0 1 0 0
    points InsertPoint 1 0 0 0
    points InsertPoint 2 0 10 0
    points InsertPoint 3 12 10 0
    points InsertPoint 4 12 0 0
    points InsertPoint 5 3 0 0

vtkCellArray lines
    lines InsertNextCell 6;#number of points
    lines InsertCellPoint 0
    lines InsertCellPoint 1
    lines InsertCellPoint 2
    lines InsertCellPoint 3
    lines InsertCellPoint 4
    lines InsertCellPoint 5

vtkPolyData profile
    profile SetPoints points
    profile SetLines lines

# extrude profile to make wall
#
vtkLinearExtrusionFilter extrude
    extrude SetInput profile
    extrude SetScaleFactor 8
    extrude SetVector 0 0 1
    extrude CappingOff
    
vtkPolyDataMapper map
    map SetInput [extrude GetOutput]

vtkActor wall
    wall SetMapper map
    [wall GetProperty] SetColor 0.3800 0.7000 0.1600
#[wall GetProperty] BackfaceCullingOff
#[wall GetProperty] FrontfaceCullingOn

# Add the actors to the renderer, set the background and size
#
ren1 AddActor wall
ren1 SetBackground 1 1 1

renWin SetSize 500 500

[ren1 GetActiveCamera] SetViewUp -0.181457 0.289647 0.939776
[ren1 GetActiveCamera] SetPosition 23.3125 -28.2001 17.5756
[ren1 GetActiveCamera] SetFocalPoint 6 5 4
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] ComputeViewPlaneNormal

renWin Render
# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName walls.tcl.ppm
#renWin SaveImageAsPPM


