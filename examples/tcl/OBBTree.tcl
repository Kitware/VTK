# get the interactor ui
source vtkInt.tcl

#vtkSTLReader reader;
#  reader SetFilename ../../data/42400-IDGH.stl;
vtkCyberReader reader;
  reader SetFilename ../../data/fran_cut
  reader DebugOn;
vtkPolyMapper dataMapper;
  dataMapper SetInput [reader GetOutput];
vtkActor model;
  model SetMapper dataMapper;
  [model GetProperty] SetColor 1 0 0;
  model VisibilityOff;

vtkOBBFilter boxes;
  boxes SetInput [reader GetOutput];
  boxes SetMaxLevel 12;
  boxes DebugOn;
vtkPolyMapper boxMapper;
  boxMapper SetInput [boxes GetOutput];
#  boxMapper SetInput [boxes GetOutput 4];
  boxMapper DebugOn;
vtkActor boxActor;
  boxActor SetMapper boxMapper;
  [boxActor GetProperty] SetWireframe;

vtkRenderMaster rm;
set renWin [rm MakeRenderWindow];
set ren1 [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors model;
$ren1 AddActors boxActor;
$ren1 SetBackground 0.1 0.2 0.4;
$renWin SetSize 500 500;
$renWin Render;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};
$iren Initialize;

# prevent the tk window from showing up then start the event loop
wm withdraw .

