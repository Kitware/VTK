# Use Claw to find a path through the colon. 
# Show the internal model of the colon.


source define.tcl

set sliceNumber 176
set sliceMax 177


# Image pipeline
vtkImageShortReader reader;
reader SetFilePrefix "/projects/lorensen/colon/slices/scolon";
reader ReleaseDataFlagOff;
reader SwapBytesOn;
reader SetDimensions 256 256 178;
reader SetPixelMask 0x7fff;
reader SetOutputScalarType $VTK_SHORT;
reader DebugOn;


# Set up the path planner
vtkImageDraw region;
region SetScalarType $VTK_SHORT;
region SetExtent 0 255 0 255 0 177;
[reader GetOutput] UpdateRegion region;

#region DrawSegment 0 216 255 216;
#region DrawSegment 107 0 107 255;

vtkImageStateSpace space;
space SetNumberOfDimensions 3;
space SetRegion region;
space SetThreshold 600;


puts "Generating path ------------------------------------";

vtkClaw claw;
#claw SetChildFraction 0.70;
#claw SetNeighborFraction 0.75;
claw ClearSearchStrategies;
claw AddSearchStrategy $VTK_CLAW_NEAREST_NETWORK;
claw AddSearchStrategy $VTK_CLAW_PIONEER_LOCAL;
claw SetStateSpace space;
claw SetStartState 69 118 11;
claw SetGoalState 107 216 176;
#claw DebugOn;
claw GeneratePath;
puts "Smoothing ------------------------------------------------";
#claw DebugOn;
claw SmoothPath 20;
claw SavePath "ClawColonSmooth.path";

region ReleaseData;





# Display the path
# First create the render master
vtkRenderMaster rm;

# Now create the RenderWindow and Renderer.
#
set renWin [rm MakeRenderWindow];
set ren1   [$renWin MakeRenderer];
set iren [$renWin MakeRenderWindowInteractor];


vtkSphereSource sphere;
sphere SetRadius 1.0;

vtkGlyph3D glyph;
glyph SetSource [sphere GetOutput];
glyph ScalingOn;
#glyph SetScaleFactor 1.333;
glyph ScaleByScalar;
glyph OrientOff;
glyph SetInput [space GetPathPolyData claw];

vtkPolyMapper pathMapper;
pathMapper SetInput [glyph GetOutput];

vtkActor pathActor;
pathActor SetMapper pathMapper;
[pathActor GetProperty] SetColor 0 1 0;




vtkGlyph3D collisionGlyph;
collisionGlyph SetSource [sphere GetOutput];
collisionGlyph ScalingOff;
collisionGlyph OrientOff;
collisionGlyph SetInput [space GetCollisionPolyData claw];

vtkPolyMapper collisionMapper;
collisionMapper SetInput [collisionGlyph GetOutput];

vtkActor collisionActor;
collisionActor SetMapper collisionMapper;
[collisionActor GetProperty] SetColor 1 0 0;



# Add the actors to the renderer, set the background and size
#
$ren1 AddActors pathActor;
$ren1 AddActors collisionActor;
$renWin SetSize 500 500;
#$renWin SetSize 1000 1000;
$ren1 SetBackground 0.1 0.2 0.4;
$iren Initialize;

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract};
[$ren1 GetActiveCamera] Zoom 1.5;
$renWin Render;
#$renWin SetFilename "complexV.tcl.ppm";
#$renWin SaveImageAsPPM;

# prevent the tk window from showing up then start the event loop
wm withdraw .





















