# create selected streamlines in arteries
source colors.tcl
source vtkInclude.tcl

vtkRenderMaster rm;

set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# create pipeline
#
vtkStructuredPointsReader reader;
    reader SetFilename "../../data/carotid.vtk"
vtkPointSource psource;
    psource SetNumberOfPoints 25;
    psource SetCenter 133.1 116.3 5.0;
    psource SetRadius 2.0;
vtkThresholdPoints threshold;
    threshold SetInput [reader GetOutput];
    threshold ThresholdByUpper 275;
vtkStreamLine streamers;
    streamers SetInput [reader GetOutput];
    streamers SetSource [psource GetOutput];
    streamers SetMaximumPropagationTime 100.0;
    streamers SetIntegrationStepLength 0.2;
    streamers SpeedScalarsOn;
    streamers SetTerminalSpeed .1;
vtkTubeFilter tubes;
    tubes SetInput [streamers GetOutput];
    tubes SetRadius 0.3;
    tubes SetNumberOfSides 6;
    tubes SetVaryRadius $VTK_VARY_RADIUS_OFF;
vtkLookupTable lut;
    lut SetHueRange .667 0.0;
    lut Build;
vtkPolyMapper streamerMapper;
    streamerMapper SetInput [tubes GetOutput];
    streamerMapper SetScalarRange 2 10;
    streamerMapper SetLookupTable lut;
vtkActor streamerActor;
    streamerActor SetMapper streamerMapper;

# contours of speed
vtkContourFilter iso;
    iso SetInput [reader GetOutput];
    iso SetValue 0 190;
vtkPolyMapper isoMapper;
    isoMapper SetInput [iso GetOutput];
    isoMapper ScalarsVisibleOff;
vtkActor isoActor;
    isoActor SetMapper isoMapper;
    [isoActor GetProperty] SetWireframe;
    [isoActor GetProperty] SetOpacity 0.25;

# outline
vtkOutlineFilter outline;
    outline SetInput [reader GetOutput];
vtkPolyMapper outlineMapper;
    outlineMapper SetInput [outline GetOutput];
vtkActor outlineActor;
    outlineActor SetMapper outlineMapper;
set outlineProp [outlineActor GetProperty];
eval $outlineProp SetColor 0 0 0;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor;
$ren1 AddActors streamerActor;
$ren1 AddActors isoActor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;

vtkCamera cam1;
  cam1 SetClippingRange 17.4043 870.216
  cam1 SetFocalPoint 136.71 104.025 23
  cam1 SetPosition 204.747 258.939 63.7925
  cam1 CalcViewPlaneNormal;
  cam1 SetViewUp -0.102647 -0.210897 0.972104
  cam1 Zoom 1.6
$ren1 SetActiveCamera cam1;

$iren Initialize;

# render the image
#
$iren SetUserMethod {
  commandloop "puts -nonewline vtki>"; puts cont};

$renWin Render;
#$renWin SetFilename "streamV.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .
