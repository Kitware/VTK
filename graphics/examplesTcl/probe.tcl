# get the interactor ui
source vtkInt.tcl
source "colors.tcl"
# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# cut data
vtkPLOT3DReader pl3d;
    pl3d SetXYZFileName "../../data/combxyz.bin"
    pl3d SetQFileName "../../data/combq.bin"
    pl3d SetScalarFunctionNumber 100;
    pl3d SetVectorFunctionNumber 202;
    pl3d Update;
vtkPlane plane;
    eval plane SetOrigin [[pl3d GetOutput] GetCenter];
    plane SetNormal -0.287 0 0.9579;
vtkCutter planeCut;
    planeCut SetInput [pl3d GetOutput];
    planeCut SetCutFunction plane;
vtkProbeFilter probe;
    probe SetInput [planeCut GetOutput];
    probe SetSource [pl3d GetOutput];
vtkDataSetMapper cutMapper;
    cutMapper SetInput [probe GetOutput];
    eval cutMapper SetScalarRange \
      [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange];
vtkActor cutActor;
    cutActor SetMapper cutMapper;

#extract plane
vtkStructuredGridGeometryFilter compPlane;
    compPlane SetInput [pl3d GetOutput];
    compPlane SetExtent 0 100 0 100 9 9;
vtkPolyMapper planeMapper;
    planeMapper SetInput [compPlane GetOutput];
    planeMapper ScalarsVisibleOff;
vtkActor planeActor;
    planeActor SetMapper planeMapper;
    [planeActor GetProperty] SetWireframe;
    [planeActor GetProperty] SetColor 0 0 0;

#outline
vtkStructuredGridOutlineFilter outline;
    outline SetInput [pl3d GetOutput];
vtkPolyMapper outlineMapper;
    outlineMapper SetInput [outline GetOutput];
vtkActor outlineActor;
    outlineActor SetMapper outlineMapper;
set outlineProp [outlineActor GetProperty];
eval $outlineProp SetColor 0 0 0;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor;
$ren1 AddActors planeActor;
$ren1 AddActors cutActor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;

set cam1 [$ren1 GetActiveCamera];
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 CalcViewPlaneNormal;
$cam1 SetViewUp -0.16123 0.264271 0.950876
$iren Initialize;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};

#$renWin SetFileName "probe.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .



