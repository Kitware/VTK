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
vtkContourFilter iso;
    iso SetInput [reader GetOutput];
    iso SetValue 0 128;
vtkPolyMapper isoMapper;
    isoMapper SetInput [iso GetOutput];
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

$renWin Render;
#$renWin SetFilename "ironPIso.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .


