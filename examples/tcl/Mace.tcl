# this is a tcl version of the Mace example
# include get the vtk interactor ui
source vtkInt.tcl

# Create the render master
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
vtkActor sphereActor;
    sphereActor SetMapper sphereMapper;
    [sphereActor GetProperty] BackfaceCullingOn;

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
vtkActor spikeActor;
    spikeActor SetMapper spikeMapper;
    [spikeActor GetProperty] BackfaceCullingOn;

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors sphereActor;
$ren1 AddActors spikeActor;
$ren1 SetBackground 0.1 0.2 0.4;
$renWin SetSize 450 450;

# Get handles to some useful objects
#
$iren SetUserMethod {wm deiconify .vtkInteract};
$iren Initialize;
$renWin Render;
set cam1 [$ren1 GetActiveCamera];
set sphereProp [sphereActor GetProperty];
set spikeProp [spikeActor GetProperty];

# Create a loop to draw some pretty pictures
# 
for {set i 0} {$i < 360} {incr i; incr i} {
  $cam1 Azimuth 5;
  $ren1 SetBackground 0.6 0.0 [expr (360.0 - $i) / 400.0];
  $sphereProp SetColor \
    0.5 [expr $i / 440.0] [expr (360.0 - $i) / 440.0];
  $spikeProp SetColor \
    [expr (360.0 - $i) / 440.0] 0.5 [expr $i / 440.0];
  $renWin Render;
}

# prevent the tk window from showing up then start the event loop
wm withdraw .


