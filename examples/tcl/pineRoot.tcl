# get the interactor ui and colors
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
vtkMCubesReader reader;
    reader SetFilename "../../data/pineRoot/pine_root.tri";
    reader FlipNormalsOff;
vtkPolyMapper isoMapper;
    isoMapper SetInput [reader GetOutput];
    isoMapper ScalarsVisibleOff;
vtkActor isoActor;
    isoActor SetMapper isoMapper;
    eval [isoActor GetProperty] SetColor $raw_sienna;

vtkOutlineFilter outline;
    outline SetInput [reader GetOutput];
vtkPolyMapper outlineMapper;
    outlineMapper SetInput [outline GetOutput];
vtkActor outlineActor;
    outlineActor SetMapper outlineMapper;
    [outlineActor GetProperty] SetColor 0 0 0;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor;
$ren1 AddActors isoActor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;
eval $ren1 SetBackground $slate_grey;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};

set cam [$ren1 GetActiveCamera];
  $cam SetFocalPoint 40.6018 37.2813 50.1953
  $cam SetPosition 40.6018 -280.533 47.0172;
  $cam CalcViewPlaneNormal;
  $cam SetClippingRange 26.1073 1305.36;
  $cam SetViewAngle 20.9219;
  $cam SetViewUp 0.0 0.0 1.0
$renWin Render;
#$renWin SetFilename "pineRoot.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .
