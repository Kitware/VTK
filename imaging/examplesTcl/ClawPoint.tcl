# This script test the Claw planner by moving a point through an image.


source vtkImageInclude.tcl



# Image pipeline

# Make the WorkSpace
vtkImageDraw workspace;
workspace SetScalarType $VTK_SHORT
workspace SetExtent 0 550 0 550;
workspace SetDrawValue 0;
workspace FillBox 0 550 0 550;
workspace SetDrawValue 500;
workspace FillBox 10 500 50 100;
workspace FillBox 100 150 50 350;
workspace FillBox 300 500 50 500;

# Make the state space (and robot)
vtkImageStateSpace space;
space SetStateDimensionality 2;
space SetRegion workspace;
space SetCollisionValue 0;


set viewer [space GetViewer];
set canvas [space GetCanvas];

$canvas SetDrawColor 50 50 50;
$canvas DrawPoint 450 450;
$canvas DrawPoint 140 290;

$viewer Render;


# Set up the path planner
vtkClaw claw;
claw PruneCollisionsOff;
claw ClearSearchStrategies;
claw AddSearchStrategy $VTK_CLAW_NEAREST_NETWORK;
#claw AddSearchStrategy $VTK_CLAW_PIONEER_LOCAL;
claw SetSamplePeriod 100;
claw CallBacksOn;
claw SetStateSpace space
space SetPlanner claw;
claw SetStartState 450 450;
claw SetGoalState 140 290;
puts "Finding path ---------------------------------------------";
claw GeneratePath;


puts "Exploring ------------------------------------------------";
claw SetChildFraction 0.6;
claw SetNeighborFraction 0.7;
claw ExplorePath 5;
space SampleCallBack claw;

puts "Smoothing ------------------------------------------------";
claw SmoothPath 3;
space SampleCallBack claw;


$canvas SetDrawColor 0.5 0.5 0.5;
space DrawPath claw;
$viewer Render;


wm withdraw .





