# get the interactor ui
source vtkInt.tcl

# lines make a nice test
vtkLineSource line1;
  line1 SetPoint1 0 0 0;
  line1 SetPoint2 1 0 0;
  line1 SetResolution 1000;
vtkLineSource line2;
  line2 SetPoint1 0 0 0;
  line2 SetPoint2 1 1 1;
  line2 SetResolution 1000;
#vtkAppendPolyData source;
#  source AddInput [line1 GetOutput];
#  source AddInput [line2 GetOutput];

vtkSTLReader source;
  source SetFilename ../../data/42400-IDGH.stl;
#vtkCyberReader source;
#  source SetFilename ../../data/fran_cut
  source DebugOn;
vtkPolyMapper dataMapper;
  dataMapper SetInput [source GetOutput];
vtkActor model;
  model SetMapper dataMapper;
  [model GetProperty] SetColor 1 0 0;
#  model VisibilityOff;

#vtkPointLocator locator;
#vtkOBBTree locator;
vtkCellLocator locator;
  locator SetMaxLevel 4;
  locator AutomaticOff;
  locator DebugOn;
vtkSpatialRepFilter boxes;
  boxes SetInput [source GetOutput];
  boxes SetSpatialRep locator;
vtkPolyMapper boxMapper;
  boxMapper SetInput [boxes GetOutput];
#  boxMapper SetInput [boxes GetOutput 2];
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

