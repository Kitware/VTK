# Example demonstrates use of vtkCastToConcrete
#
source vtkInt.tcl

vtkSphereSource sphere;
    sphere SetPhiResolution 12;
    sphere SetThetaResolution 12;
sphere DebugOn;

vtkElevationFilter colorIt;
  colorIt SetInput [sphere GetOutput];
  colorIt SetLowPoint 0 0 -1;
  colorIt SetHighPoint 0 0 1;

vtkCastToConcrete cast;
  cast SetInput [colorIt GetOutput];
  cast DebugOn;

vtkPolyMapper mapper;
  mapper SetInput [cast GetPolyDataOutput] 

vtkActor actor;
  actor SetMapper mapper;

vtkRenderMaster rm;
set renWin [rm MakeRenderWindow];
set iren [$renWin MakeRenderWindowInteractor];
set renderer [$renWin MakeRenderer];

$renderer AddActors actor;
$renderer SetBackground 1 1 1;
$renWin SetSize 450 450;

$iren SetUserMethod {wm deiconify .vtkInteract};
$iren Initialize;
$renWin Render;

wm withdraw .
