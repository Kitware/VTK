# get the interactor ui
source vtkInt.tcl
source colors.tcl

# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# create pipeline
#
vtkVolume16Reader v16;
    v16 SetDataDimensions 128 128 ;
    [v16 GetOutput] SetOrigin 0.0 0.0 0.0;
    v16 SetFileTypeLittleEndian;
    v16 SetFilePrefix "../../data/headsq/half";
    v16 SetImageRange 45 45;
    v16 SetDataAspectRatio 1.6 1.6 1.5;
vtkContourFilter iso;
    iso SetInput [v16 GetOutput];
    iso GenerateValues 12 500 1150;
vtkPolyMapper isoMapper;
    isoMapper SetInput [iso GetOutput];
    isoMapper ScalarsVisibleOff;
vtkActor isoActor;
    isoActor SetMapper isoMapper;
    eval [isoActor GetProperty] SetColor $black;

vtkOutlineFilter outline;
    outline SetInput [v16 GetOutput];
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

$iren Initialize;

#$renWin SetFileName "headSlic.tcl.ppm";
#$renWin SaveImageAsPPM;

$iren SetUserMethod {wm deiconify .vtkInteract};

# prevent the tk window from showing up then start the event loop
wm withdraw .


