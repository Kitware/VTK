# Created oriented text
source vtkInt.tcl

# pipeline
vtkAxes axes;
    axes SetOrigin 0 0 0;
vtkPolyMapper axesMapper;
    axesMapper SetInput [axes GetOutput];
vtkActor axesActor;
    axesActor SetMapper axesMapper;

vtkVectorText text;
    text SetText "Origin";
vtkPolyMapper textMapper;
    textMapper SetInput [text GetOutput];
vtkFollower textActor;
    textActor SetMapper textMapper;
    textActor SetScale 0.0025 0.0025 0.0025
    textActor AddPosition 0 -0.1 0;

# create graphics stuff
vtkRenderMaster rm;
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

$ren1 AddActors axesActor;
$ren1 AddActors textActor;

$renWin Render;
textActor SetCamera [$ren1 GetActiveCamera];

$iren SetUserMethod {wm deiconify .vtkInteract};
$iren Initialize;

wm withdraw .;
