#create triangle strip - won't see anything with backface culling on

# get the interactor ui
source vtkInt.tcl

# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren   [$renWin MakeRenderWindowInteractor];

# create triangle strip
#
vtkFloatPoints points;
    points InsertPoint 0 0.0 0.0 0.0;
    points InsertPoint 1 0.0 1.0 0.0;
    points InsertPoint 2 1.0 0.0 0.0;
    points InsertPoint 3 1.0 1.0 0.0;
    points InsertPoint 4 2.0 0.0 0.0;
    points InsertPoint 5 2.0 1.0 0.0;
    points InsertPoint 6 3.0 0.0 0.0;
    points InsertPoint 7 3.0 1.0 0.0;
vtkCellArray strips;
    strips InsertNextCell 8;#number of points
    strips InsertCellPoint 0;
    strips InsertCellPoint 1;
    strips InsertCellPoint 2;
    strips InsertCellPoint 3;
    strips InsertCellPoint 4;
    strips InsertCellPoint 5;
    strips InsertCellPoint 6;
    strips InsertCellPoint 7;
vtkPolyData profile;
    profile SetPoints points;
    profile SetStrips strips;

vtkPolyMapper map;
    map SetInput profile;

vtkActor strip;
    strip SetMapper map;
    [strip GetProperty] SetColor 0.3800 0.7000 0.1600;
    [strip GetProperty] BackfaceCullingOff;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors strip;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;
$renWin Render;

#$renWin SetFileName "strip.tcl.ppm";
#$renWin SaveImageAsPPM;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .



