# Simple viewer for images.

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
exit




















