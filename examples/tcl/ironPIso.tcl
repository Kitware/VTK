# get the interactor ui
source vtkInt.tcl
source "colors.tcl"
# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# create pipeline
#
vtkStructuredPointsReader reader;
    reader SetFilename "../../data/ironProt.vtk"
    reader DebugOn;
vtkContourFilter iso;
    iso SetInput [reader GetOutput];
    iso SetValue 0 128;
vtkCleanPolyData clean;
    clean SetInput [iso GetOutput];
vtkPolyNormals normals;
    normals SetInput [clean GetOutput];
    normals SetFeatureAngle 45;
vtkPolyMapper isoMapper;
    isoMapper SetInput [normals GetOutput];
    isoMapper ScalarsVisibleOff;
vtkActor isoActor;
    isoActor SetMapper isoMapper;
set isoProp [isoActor GetProperty];
eval $isoProp SetColor $bisque;

vtkOutlineFilter outline;
    outline SetInput [reader GetOutput];
vtkPolyMapper outlineMapper;
    outlineMapper SetInput [outline GetOutput];
vtkActor outlineActor;
    outlineActor SetMapper outlineMapper;
set outlineProp [outlineActor GetProperty];
#eval $outlineProp SetColor 0 0 0;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor;
$ren1 AddActors isoActor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;
$ren1 SetBackground 0.1 0.2 0.4;
$renWin DoubleBufferOff;
$iren Initialize;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};

vtkCamera cam1;
    cam1 SetClippingRange 19.1589 957.946;
    cam1 SetFocalPoint 33.7014 26.706 30.5867;
    cam1 SetPosition 150.841 89.374 -107.462;
    cam1 CalcViewPlaneNormal;
    cam1 SetViewUp -0.190015 0.944614 0.267578;
$ren1 SetActiveCamera cam1;

$renWin Render;
#$renWin SetFilename "color9d.ppm";
#$renWin SaveImageAsPPM;
puts "Done";

# prevent the tk window from showing up then start the event loop
wm withdraw .


