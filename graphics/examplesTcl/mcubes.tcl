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
v16 SetDataDimensions 128 128;
[v16 GetOutput] SetOrigin 0.0 0.0 0.0;
v16 SwapBytesOn;
v16 SetFilePrefix "../../data/headsq/half";
#v16 SetImageRange 19 24;
v16 SetImageRange 1 93;
v16 SetDataAspectRatio 1.6 1.6 1.5;
v16 Update;

vtkMarchingCubes iso;
iso SetInput [v16 GetOutput];
iso SetValue 0 1150;

vtkPolyMapper isoMapper;
isoMapper SetInput [iso GetOutput];
isoMapper ScalarsVisibleOff;

vtkActor isoActor;
isoActor SetMapper isoMapper;
eval [isoActor GetProperty] SetColor $antique_white;

vtkOutlineFilter outline;
    outline SetInput [v16 GetOutput];
vtkPolyMapper outlineMapper;
    outlineMapper SetInput [outline GetOutput];
vtkActor outlineActor;
    outlineActor SetMapper outlineMapper;
    outlineActor VisibilityOff;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor;
$ren1 AddActors isoActor;
$ren1 SetBackground 0.2 0.3 0.4;
$renWin SetSize 450 450;
[$ren1 GetActiveCamera] Elevation 90;
[$ren1 GetActiveCamera] SetViewUp 0 0 -1;
[$ren1 GetActiveCamera] Azimuth 180;
$iren Initialize;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};
#$renWin SetFilename "mcubes.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .


