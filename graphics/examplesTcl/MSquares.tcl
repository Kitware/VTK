## Test the marching squares contouring class

source vtkInt.tcl

# Quadric definition
vtkQuadric quadric;
  quadric SetCoefficients .5 1 .2 0 .1 0 0 .2 0 0;

vtkSampleFunction sample;
  sample SetSampleDimensions 30 30 30;
  sample SetImplicitFunction quadric;

vtkExtractVOI extract;
  extract SetInput [sample GetOutput];
  extract SetVOI 0 29 0 29 15 15;
  extract SetSampleRate 1 2 3;

vtkContourFilter contours;
  contours SetInput [extract GetOutput];
  contours GenerateValues 13 0.0 1.2;

vtkPolyMapper contMapper;
  contMapper SetInput [contours GetOutput];
  contMapper SetScalarRange 0.0 1.2;

vtkActor contActor;
  contActor SetMapper contMapper;

# Create outline
vtkOutlineFilter outline;
  outline SetInput [sample GetOutput];

vtkPolyMapper outlineMapper;
  outlineMapper SetInput [outline GetOutput];

vtkActor outlineActor;
  outlineActor SetMapper outlineMapper;
  eval [outlineActor GetProperty] SetColor 0 0 0;

# create graphics objects
vtkRenderMaster rm;

# create a window to render into
set renWin [rm MakeRenderWindow];

# create a renderer
set ren1 [$renWin MakeRenderer];

# interactiver renderer catches mouse events (optional)
set iren [$renWin MakeRenderWindowInteractor];
$ren1 SetBackground 1 1 1;
$ren1 AddActors contActor;
$ren1 AddActors outlineActor;

[$ren1 GetActiveCamera] Zoom 1.5;
$iren SetUserMethod {wm deiconify .vtkInteract};
$iren Initialize;;

#$renWin SetFileName MSquares.tcl.ppm;
#$renWin SaveImageAsPPM;

wm withdraw .;
