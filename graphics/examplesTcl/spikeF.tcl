# this is a tcl version of old spike-face
# get the interactor ui
source vtkInt.tcl
# First create the render master
#
vtkRenderMaster rm;

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];

# create a cyberware source
#
vtkCyberReader cyber;
    cyber SetFilename "../../data/fran_cut"
vtkPolyNormals normals;
    normals SetInput [cyber GetOutput];
vtkPolyMapper cyberMapper;
    cyberMapper SetInput [normals GetOutput];
vtkActor cyberActor;
    cyberActor SetMapper cyberMapper;
eval [cyberActor GetProperty] SetColor 1.0 0.49 0.25;

# create the spikes using a cone source and a subset of cyber points
#
vtkMaskPoints ptMask;
    ptMask SetInput [normals GetOutput];
    ptMask SetOnRatio 100;
    ptMask RandomModeOn;
vtkConeSource cone;
    cone SetResolution 6;
vtkTransform transform;
    transform Translate 0.5 0.0 0.0;
vtkTransformPolyFilter transformF;
    transformF SetInput [cone GetOutput];
    transformF SetTransform transform;
vtkGlyph3D glyph;
    glyph SetInput [ptMask GetOutput];
    glyph SetSource [transformF GetOutput];
    glyph UseNormal;
    glyph ScaleByVector;
    glyph SetScaleFactor 0.004;
vtkPolyMapper spikeMapper;
    spikeMapper SetInput [glyph GetOutput];
vtkActor spikeActor;
    spikeActor SetMapper spikeMapper;
eval [spikeActor GetProperty] SetColor 0.0 0.79 0.34;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors cyberActor;
$ren1 AddActors spikeActor;
$ren1 SetBackground 1 1 1;
$renWin SetSize 500 500;
#$renWin SetSize 1000 1000;
$ren1 SetBackground 0.1 0.2 0.4;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};
set cam1 [$ren1 GetActiveCamera];
set sphereProp [cyberActor GetProperty];
set spikeProp [spikeActor GetProperty];

# do stereo example
$cam1 Zoom 1.4;
$cam1 Azimuth 110;
$renWin Render;
#$renWin SetFilename "spikeF.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .


