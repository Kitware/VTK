# create ice-cream cone
# get the interactor ui
source vtkInt.tcl
source "colors.tcl"
# First create the render master
vtkRenderMaster rm;

set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# create implicit function primitives
vtkCone cone;
    cone SetAngle 8.0;
vtkPlane vertPlane;
    vertPlane SetOrigin 0 0 0;
    vertPlane SetNormal 0 0 -1;
vtkPlane basePlane;
    basePlane SetOrigin 0 0 1;
    basePlane SetNormal 0 0 1;
vtkSphere iceCream;
    iceCream SetCenter 0 0 1.333;
    iceCream SetRadius 0.5;
vtkSphere bite;
    bite SetCenter 0.5 0 1.5;
    bite SetRadius 0.25;

# combine primitives to build ice-cream cone
vtkImplicitBoolean theCone;
    theCone SetOperationType 1;#intersection
    theCone AddFunction cone;
    theCone AddFunction vertPlane;
    theCone AddFunction basePlane;

vtkImplicitBoolean theCream;
    theCream SetOperationType 2;#difference
    theCream AddFunction iceCream;
    theCream AddFunction bite;

# iso-surface to create geometry
vtkSampleFunction theConeSample;
    theConeSample SetImplicitFunction theCone;
    theConeSample SetModelBounds -1.25 1.25 -1.25 1.25 -1 1.5;
    theConeSample SetSampleDimensions 60 60 60;
vtkContourFilter theConeSurface;
    theConeSurface SetInput [theConeSample GetOutput];
    theConeSurface SetValue 0 0.0;
vtkPolyMapper coneMapper;
    coneMapper SetInput [theConeSurface GetOutput];
    coneMapper ScalarsVisibleOff;
vtkActor coneActor;
    coneActor SetMapper coneMapper;
    eval [coneActor GetProperty] SetColor $chocolate;

# iso-surface to create geometry
vtkSampleFunction theCreamSample;
    theCreamSample SetImplicitFunction theCream;
    theCreamSample SetModelBounds -1.25 1.25 -1.25 1.25 0 2.5;
    theCreamSample SetSampleDimensions 60 60 60;
vtkContourFilter theCreamSurface;
    theCreamSurface SetInput [theCreamSample GetOutput];
    theCreamSurface SetValue 0 0.0;
vtkPolyMapper creamMapper;
    creamMapper SetInput [theCreamSurface GetOutput];
    creamMapper ScalarsVisibleOff;
vtkActor creamActor;
    creamActor SetMapper creamMapper;
    eval [creamActor GetProperty] SetColor $mint;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors coneActor;
$ren1 AddActors creamActor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;
[$ren1 GetActiveCamera] Azimuth 90;
$iren Initialize;

#$renWin SetFilename "iceCream.tcl.ppm";
#$renWin SaveImageAsPPM;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};
$renWin Render;

# prevent the tk window from showing up then start the event loop
wm withdraw .
