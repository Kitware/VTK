# Example demonstrates use of vtkCastToConcrete
#
source vtkInt.tcl

vtkSphereSource sphere;
    sphere SetPhiResolution 12;
    sphere SetThetaResolution 12;

vtkElevationFilter colorIt;
  colorIt SetInput [sphere GetOutput];
  colorIt SetLowPoint 0 0 -1;
  colorIt SetHighPoint 0 0 1;

vtkCastToConcrete cast;
  cast SetInput [colorIt GetOutput];

vtkPolyMapper mapper;
  mapper SetInput [cast GetPolyDataOutput] 

vtkActor actor;
  actor SetMapper mapper;

vtkRenderMaster rm;
set renWin [rm MakeRenderWindow];
set iren [$renWin MakeRenderWindowInteractor];
set ren1 [$renWin MakeRenderer];

$ren1 AddActors actor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 400 400;
[$ren1 GetActiveCamera] Zoom 1.4;

$iren SetUserMethod {wm deiconify .vtkInteract};
$iren Initialize;

#$renWin SetFileName ColorSph.tcl.ppm;
#$renWin SaveImageAsPPM;

wm withdraw .
