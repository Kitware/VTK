# get the interactor ui
source vtkInt.tcl

# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren   [$renWin MakeRenderWindowInteractor];

# create spring profile (a circle)
#
vtkFloatPoints points;
    points InsertPoint 0 1.0 0.0 0.0;
    points InsertPoint 1 1.0732 0.0 -0.1768;
    points InsertPoint 2 1.25 0.0 -0.25;
    points InsertPoint 3 1.4268 0.0 -0.1768;
    points InsertPoint 4 1.5 0.0 0.00;
    points InsertPoint 5 1.4268 0.0 0.1768;
    points InsertPoint 6 1.25 0.0 0.25;
    points InsertPoint 7 1.0732 0.0 0.1768;
vtkCellArray poly;
    poly InsertNextCell 8;#number of points
    poly InsertCellPoint 0;
    poly InsertCellPoint 1;
    poly InsertCellPoint 2;
    poly InsertCellPoint 3;
    poly InsertCellPoint 4;
    poly InsertCellPoint 5;
    poly InsertCellPoint 6;
    poly InsertCellPoint 7;
vtkPolyData profile;
    profile SetPoints points;
    profile SetPolys poly;

# extrude profile to make spring
#
vtkRotationalExtrusionFilter extrude;
    extrude SetInput profile;
    extrude SetResolution 360;
    extrude SetTranslation 6;
    extrude SetDeltaRadius 1.0;
    extrude SetAngle 2160.0;#six revolutions
    
vtkPolyNormals normals;
    normals SetInput [extrude GetOutput];
    normals SetFeatureAngle 60;

vtkPolyMapper map;
    map SetInput [normals GetOutput];

vtkActor spring;
    spring SetMapper map;
    [spring GetProperty] SetColor 0.6902 0.7686 0.8706;
    [spring GetProperty] SetDiffuse 0.7;
    [spring GetProperty] SetSpecular 0.4;
    [spring GetProperty] SetSpecularPower 20;
    [spring GetProperty] BackfaceCullingOn;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors spring;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;
$renWin Render;

set cam1 [$ren1 GetActiveCamera];
#$cam1 SetClippingRange 3.95297 50
#$cam1 SetFocalPoint 9.71821 0.458166 29.3999
#$cam1 SetPosition 2.7439 -37.3196 38.7167
#$cam1 CalcViewPlaneNormal;
#$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};

$renWin Render;

# prevent the tk window from showing up then start the event loop
wm withdraw .

$iren Start;



