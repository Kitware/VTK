# Use Claw to find a path through the colon. 
# Show the internal model of the colon.


source ../imaging/examplesTcl/vtkImageInclude.tcl

set sliceNumber 176
set sliceMax 177


# Image pipeline
vtkImageSeriesReader reader;
reader SetFilePrefix "/projects/lorensen/colon/slices/scolon";
reader ReleaseDataFlagOff;
reader SwapBytesOn;
reader SetDataDimensions 256 256 178;
reader SetPixelMask 0x7fff;
reader SetOutputScalarType $VTK_SHORT;
reader DebugOn;

vtkImageThreshold thresh;
thresh ThresholdByLower 600;
thresh SetInValue 255.0;
thresh SetOutValue 0.0;
thresh SetInput [reader GetOutput];
thresh SetOutputScalarType $VTK_UNSIGNED_CHAR;
thresh SetInputMemoryLimit 5000;

# Set up the path planner
vtkImagePaint region;
region SetExtent 0 255 0 255 0 177;
[thresh GetOutput] UpdateRegion region;

#region DrawSegment 0 216 255 216;
#region DrawSegment 107 0 107 255;

vtkImageStateSpace space;
space SetStateDimensionality 3;
space SetRegion region;


puts "Generating path ------------------------------------";

vtkClaw claw;
claw ClearSearchStrategies;
claw AddSearchStrategy $VTK_CLAW_NEAREST_NETWORK;
claw AddSearchStrategy $VTK_CLAW_PIONEER_LOCAL;
claw SetStateSpace space;
claw SetStartState 69 118 11;
claw SetGoalState 107 216 176;
claw GeneratePath;

puts "Exploring ------------------------------------------------";
claw ClearSearchStrategies;
claw AddSearchStrategy $VTK_CLAW_NEAREST_NETWORK;
claw SetChildFraction 0.7
claw SetNeighborFraction 0.8
puts 1
claw ExplorePath 1;

puts 2
claw SetChildFraction 0.6;
claw SetNeighborFraction 0.7;
claw ExplorePath 4;

puts 3
claw SetChildFraction 0.55;
claw SetNeighborFraction 0.65;
claw ExplorePath 1;

puts 4
claw SetChildFraction 0.5;
claw SetNeighborFraction 0.65;
claw ExplorePath 1;

puts 5
claw SetChildFraction 0.45;
claw SetNeighborFraction 0.65;
claw ExplorePath 1;

puts 6
claw SetChildFraction 0.4;
claw SetNeighborFraction 0.6;
claw ExplorePath 1;

puts 7
claw SetChildFraction 0.35;
claw SetNeighborFraction 0.55;
claw ExplorePath 1;

puts 8
claw SetChildFraction 0.3;
claw SetNeighborFraction 0.53;
claw ExplorePath 1;

puts 9
claw SetChildFraction 0.5;
claw SetNeighborFraction 0.52;
claw ExplorePath 1;

puts 10
claw SetChildFraction 0.6;
claw SetNeighborFraction 0.52;
claw ExplorePath 1;

puts 11
claw SetChildFraction 0.75;
claw SetNeighborFraction 0.52;
claw ExplorePath 1;

puts 12
claw SetChildFraction 0.9;
claw SetNeighborFraction 0.52;
claw ExplorePath 1;

puts 13
claw SetChildFraction 0.95;
claw SetNeighborFraction 0.52;
claw ExplorePath 3;




puts "Smoothing ------------------------------------------------";
claw SmoothPath 5;

puts "Finished  ------------------------------------------------";

claw SavePath "colon.path"


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





















