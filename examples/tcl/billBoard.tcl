# Create a rolling billboard - requires texture support

# Get the interactor
source vtkInt.tcl

# load in the texture map
#
vtkPNMReader pnmReader;
  pnmReader SetFilename "../../data/billBoard.pgm";
vtkTexture atext;
  atext SetInput [pnmReader GetOutput];
  atext InterpolateOn;

# create a plane source and actor
vtkPlaneSource plane;
plane SetPoint1 1024 0 0;
plane SetPoint2 0 64 0;
vtkTransformTextureCoords trans;
  trans SetInput [plane GetOutput];
vtkDataSetMapper  planeMapper;
  planeMapper SetInput [trans GetOutput];
vtkActor planeActor;
  planeActor SetMapper planeMapper;
  planeActor SetTexture atext;

# Create graphics stuff
vtkRenderMaster rm;
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren   [$renWin MakeRenderWindowInteractor];

# Add the actors to the renderer, set the background and size
$ren1 AddActors planeActor;
$ren1 SetBackground 0.1 0.2 0.4;
$iren SetUserMethod {wm deiconify .vtkInteract};
$renWin SetSize 512 32;

# Setup camera
vtkCamera camera;
  camera SetClippingRange 11.8369 591.843;
  camera SetFocalPoint 512 32 0;
  camera SetPosition 512 32 118.369;
  camera SetViewAngle 30;
  camera SetViewPlaneNormal 0 0 1;
  camera SetViewUp 0 1 0;
$ren1 SetActiveCamera camera;
$renWin Render;

vtkMath math;
while {1} {
    for {set i 0} {$i < 112} {incr i} {
      eval trans AddPosition 0.01 0 0;
      after 50;
      $renWin Render;
    }
    for {set i 0} {$i < 40} {incr i} {
      eval trans AddPosition 0 0.05 0;
      after 50;
      $renWin Render;
    }
    for {set i 0} {$i < 112} {incr i} {
      eval trans AddPosition -0.01 0 0;
      after 50;
      $renWin Render;
    }
    if {[math Random 0 1] < 0.5} {trans FlipSOn;
    } else {trans FlipSOff;}
    if {[math Random 0 1] < 0.5} {trans FlipTOn;
    } else {trans FlipTOff;}
}


# prevent the tk window from showing up then start the event loop
wm withdraw .





