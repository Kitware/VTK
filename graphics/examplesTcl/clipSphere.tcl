#
# Demonstrate the use of clipping on polygonal data
#
source vtkInt.tcl
source "colors.tcl"

# create pipeline
#
vtkSphereSource sphere;
    sphere SetRadius 1;
    sphere SetPhiResolution 25;
    sphere SetThetaResolution 25;
vtkPlane plane;
    plane SetOrigin 0.25 0 0;
    plane SetNormal -1 -1 0;
vtkClipPolyData clipper;
    clipper SetInput [sphere GetOutput];
    clipper SetClipFunction plane;
    clipper GenerateClipScalarsOn
    clipper SetValue 0.0;
vtkPolyMapper clipMapper;
    clipMapper SetInput [clipper GetOutput];
    clipMapper ScalarsVisibleOff;
vtkActor clipActor;
    clipActor SetMapper clipMapper;
    eval [clipActor GetProperty] SetColor $peacock;

# Create graphics stuff
#
vtkRenderMaster rm;
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors clipActor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 400 400;
$iren Initialize;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};
$renWin SetFileName "clipSphere.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .


