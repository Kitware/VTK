# Render a sphere
#

vtkRenderMaster rm;

# create a window to render into
set renWin [rm MakeRenderWindow];

# create a renderer
set ren1 [$renWin MakeRenderer];

# interactiver renderer catches mouse events (optional)
set iren [$renWin MakeRenderWindowInteractor];

# create sphere geometry
vtkSphereSource sphere;
  sphere SetRadius 1.0;
  sphere SetThetaResolution 18;
  sphere SetPhiResolution 18;

# map to graphics library
vtkPolyMapper map;
  map SetInput [sphere GetOutput];

# actor coordinates geometry, properties, transformation
vtkActor aSphere;
  aSphere SetMapper map;
  eval [aSphere GetProperty] SetColor 0 0 1;# sphere color blue

$ren1 AddActors aSphere;
$ren1 SetBackground 1 1 1;# Background color white

# Render an image; since no lights/cameras specified, created automatically
$renWin Render;

# prevent the tk window from showing up then start the event loop
wm withdraw .

# Begin mouse interaction
$iren LightFollowCameraOff;
$iren Start;

