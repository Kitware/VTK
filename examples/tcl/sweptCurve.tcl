# get the interactor ui
source vtkInt.tcl

# create bottle profile
#
# draw the arrows
vtkFloatPoints points;
    points InsertNextPoint 1 0 0
    points InsertNextPoint 0.707 0.707 1
    points InsertNextPoint 0 1 2
    points InsertNextPoint -0.707 0.707 3
    points InsertNextPoint -1 0 4
    points InsertNextPoint -0.707 -0.707 5
    points InsertNextPoint 0 -1 6
    points InsertNextPoint 0.707 -0.707 7
vtkCellArray lines;
    lines InsertNextCell 8;
    lines InsertCellPoint 0;
    lines InsertCellPoint 1;
    lines InsertCellPoint 2;
    lines InsertCellPoint 3;
    lines InsertCellPoint 4;
    lines InsertCellPoint 5;
    lines InsertCellPoint 6;
    lines InsertCellPoint 7;
vtkPolyData profile;
    profile SetPoints points;
    profile SetLines lines;

vtkRotationalExtrusionFilter extrude;
    extrude SetInput profile;
    extrude SetAngle 360;
    
vtkPolyMapper map;
    map SetInput [extrude GetOutput];

vtkActor sweep;
    sweep SetMapper map;
    [sweep GetProperty] SetColor 0.3800 0.7000 0.1600;

# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren   [$renWin MakeRenderWindowInteractor];

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors sweep;
$ren1 SetBackground 1 1 1;
$ren1 TwoSidedLightingOn;

vtkCamera acam;
    acam SetClippingRange 1.38669 69.3345
    acam SetFocalPoint -0.0368406 0.191581 3.37003
    acam SetPosition 13.6548 2.10315 2.28369
    acam SetViewAngle 30
    acam SetViewPlaneNormal 0.98735 0.13785 -0.0783399
    acam SetViewUp 0.157669 -0.801427 0.576936

$ren1 SetActiveCamera acam;

$renWin SetSize 500 500;
$renWin Render;

$iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .

