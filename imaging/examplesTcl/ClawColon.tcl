# Simple viewer for images.


set sliceNumber 176
set sliceMax 177

set VTK_FLOAT              1
set VTK_INT                2
set VTK_SHORT              3
set VTK_UNSIGNED_SHORT     4
set VTK_UNSIGNED_CHAR      5

set VTK_IMAGE_X_AXIS             0
set VTK_IMAGE_Y_AXIS             1
set VTK_IMAGE_Z_AXIS             2
set VTK_IMAGE_TIME_AXIS          3
set VTK_IMAGE_COMPONENT_AXIS     4




# Image pipeline
vtkImageShortReader reader;
reader SetFilePrefix "/projects/lorensen/colon/slices/scolon";
reader ReleaseDataFlagOff;
reader SwapBytesOn;
reader SetDimensions 256 256 178;
reader SetPixelMask 0x7fff;
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




















