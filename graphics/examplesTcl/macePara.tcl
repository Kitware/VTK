# this is a tcl version of the Mace example
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

# create a sphere source and actor
#
vtkSphereSource sphere;
vtkPolyMapper   sphereMapper;
    sphereMapper SetInput [sphere GetOutput];
vtkLODActor sphereActor;
    sphereActor SetMapper sphereMapper;

# create the spikes using a cone source and the sphere source
#
vtkConeSource cone;
vtkGlyph3D glyph;
    glyph SetInput [sphere GetOutput];
    glyph SetSource [cone GetOutput];
    glyph UseNormal;
    glyph ScaleByVector;
    glyph SetScaleFactor 0.25;
vtkPolyMapper spikeMapper;
    spikeMapper SetInput [glyph GetOutput];
vtkLODActor spikeActor;
    spikeActor SetMapper spikeMapper;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors sphereActor;
$ren1 AddActors spikeActor;
$ren1 SetBackground 0.1 0.2 0.4;
$renWin SetSize 500 500;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};
set cam1 [$ren1 GetActiveCamera];
$cam1 ParallelProjectionOn;
$cam1 SetParallelScale 1;
$cam1 Zoom 1.4;
$iren Initialize;

#$renWin SetFilename "macePara.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .


